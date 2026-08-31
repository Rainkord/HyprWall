#pragma once
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPixmap>
#include <QImageReader>
#include <QCryptographicHash>
#include <QSaveFile>
#include <QTextStream>
#include <QSet>
#include <QDebug>

namespace ThumbCache {

inline QString cacheDir()
{
    return QDir::homePath() + QStringLiteral("/.cache/hyprwall/thumbs");
}

inline void ensureDir()
{
    QDir().mkpath(cacheDir());
}

inline QString hashKey(const QString &path, int w, int h, int fillMode = 0, int rotation = 0)
{
    QByteArray key;
    key.append(path.toUtf8());
    key.append('|');
    key.append(QByteArray::number(w));
    key.append('x');
    key.append(QByteArray::number(h));
    if (fillMode != 0) { key.append("|fm"); key.append(QByteArray::number(fillMode)); }
    if (rotation != 0) { key.append("|r");  key.append(QByteArray::number(rotation)); }
    return QCryptographicHash::hash(key, QCryptographicHash::Md5).toHex();
}

inline QString thumbPath(const QString &key)
{
    return cacheDir() + QLatin1Char('/') + key + QStringLiteral(".jpg");
}

inline QString metaPath(const QString &key)
{
    return cacheDir() + QLatin1Char('/') + key + QStringLiteral(".meta");
}

// Save a thumbnail to disk cache
inline bool save(const QString &path, int w, int h, int fillMode, int rotation, const QPixmap &px, int quality = 95)
{
    if (px.isNull()) return false;
    ensureDir();
    QString key = hashKey(path, w, h, fillMode, rotation);

    // Save pixmap as JPEG
    QString tp = thumbPath(key);
    QSaveFile sf(tp);
    if (!sf.open(QIODevice::WriteOnly)) return false;
    if (!px.save(&sf, "JPEG", quality)) { sf.cancelWriting(); return false; }
    if (!sf.commit()) return false;

    // Save metadata: original path + mtime
    QFileInfo fi(path);
    QString mp = metaPath(key);
    QSaveFile mf(mp);
    if (!mf.open(QIODevice::WriteOnly)) return false;
    QTextStream ts(&mf);
    ts << path << '\n';
    ts << fi.lastModified().toSecsSinceEpoch() << '\n';
    ts << fi.size() << '\n';
    mf.commit();
    return true;
}

// Load a thumbnail from disk cache. Returns null if miss or stale.
inline QPixmap load(const QString &path, int w, int h, int fillMode = 0, int rotation = 0)
{
    QString key = hashKey(path, w, h, fillMode, rotation);

    // Check meta file exists and is valid
    QString mp = metaPath(key);
    QFile mf(mp);
    if (!mf.open(QIODevice::ReadOnly | QIODevice::Text)) return {};

    QTextStream ts(&mf);
    QString origPath;
    qint64 origMtime = 0;
    qint64 origSize = 0;
    ts >> origPath;
    ts >> origMtime;
    ts >> origSize;
    mf.close();

    // Verify original still exists and hasn't changed
    QFileInfo fi(origPath);
    if (!fi.exists()) return {};
    if (fi.lastModified().toSecsSinceEpoch() != origMtime) return {};
    if (fi.size() != origSize) return {};

    // Load cached JPEG
    QString tp = thumbPath(key);
    QFile tf(tp);
    if (!tf.exists()) return {};
    QPixmap px(tp);
    return px;
}

// Load a large image efficiently using QImageReader::setScaledSize().
// Decodes directly to thumbnail size, avoiding the 256MB QImage limit.
// Applies rotation after scaling. Caller should handle fillMode cropping.
inline QPixmap loadScaled(const QString &path, int targetW, int targetH, int rotation = 0)
{
    QImageReader reader(path);
    reader.setAllocationLimit(0); // No memory limit — we scale before decode
    if (!reader.canRead()) return {};

    QSize origSize = reader.size();
    if (origSize.isEmpty()) return {};

    // Calculate scale factor to cover the target area (keep aspect ratio expanding)
    double scaleX = static_cast<double>(targetW) / origSize.width();
    double scaleY = static_cast<double>(targetH) / origSize.height();
    double scale = qMax(scaleX, scaleY);

    QSize scaledSize(static_cast<int>(origSize.width() * scale),
                     static_cast<int>(origSize.height() * scale));

    reader.setScaledSize(scaledSize);
    QImage img = reader.read();
    if (img.isNull()) return {};

    // Apply rotation
    if (rotation != 0) {
        QTransform t;
        t.rotate(rotation * 90.0);
        img = img.transformed(t, Qt::SmoothTransformation);
    }

    return QPixmap::fromImage(img);
}

// Remove stale cache entries whose originals are no longer in validPaths
inline void prune(const QSet<QString> &validPaths)
{
    QDir dir(cacheDir());
    QStringList metaFiles = dir.entryList(QStringList() << QStringLiteral("*.meta"), QDir::Files);
    for (const QString &mf : metaFiles) {
        QFile file(dir.filePath(mf));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        QTextStream ts(&file);
        QString origPath;
        ts >> origPath;
        file.close();

        if (!validPaths.contains(origPath)) {
            // Original removed — delete cache + meta
            QString base = mf;
            base.chop(5); // remove ".meta"
            QFile::remove(dir.filePath(base + QStringLiteral(".jpg")));
            QFile::remove(dir.filePath(mf));
        }
    }
}

// Remove all cache entries
inline void clear()
{
    QDir dir(cacheDir());
    dir.removeRecursively();
}

} // namespace ThumbCache
