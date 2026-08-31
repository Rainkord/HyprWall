#pragma once
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageWriter>
#include <QStandardPaths>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QCryptographicHash>
#include <QTextStream>

namespace ImageCache {

inline QMutex &cacheMutex()
{
    static QMutex m;
    return m;
}

inline QString compressedDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                  + "/hyprwall/compressed";
    QDir().mkpath(dir);
    return dir;
}

// MD5-based filename to avoid collisions between files with same basename
inline QString compressedPath(const QString &originalPath)
{
    QByteArray hash = QCryptographicHash::hash(
        originalPath.toUtf8(), QCryptographicHash::Md5).toHex(8);
    return compressedDir() + "/" + hash + ".jpg";
}

// Sidecar file stores original path for prune() — O(n) instead of O(n*m)
inline QString metaPath(const QString &originalPath)
{
    return compressedPath(originalPath) + ".meta";
}

inline bool isStale(const QString &originalPath)
{
    QString cp = compressedPath(originalPath);
    QFileInfo origFi(originalPath);
    QFileInfo compFi(cp);
    if (!compFi.exists()) return true;
    return compFi.lastModified() < origFi.lastModified();
}

// Create JPEG q70 copy at original resolution. Returns compressed path.
// Returns original path on failure (graceful degradation).
inline QString ensureCompressed(const QString &originalPath)
{
    if (originalPath.isEmpty()) return originalPath;

    // Skip non-image files
    QString ext = QFileInfo(originalPath).suffix().toLower();
    static const QStringList imgExts = {"jpg","jpeg","png","bmp","webp","tiff","gif"};
    if (!imgExts.contains(ext)) return originalPath;

    QMutexLocker lock(&cacheMutex());
    QString cp = compressedPath(originalPath);
    if (!isStale(originalPath)) return cp;

    QImage img(originalPath);
    if (img.isNull()) {
        qWarning() << "ImageCache: failed to load" << originalPath;
        return originalPath;
    }

    // Write to temp file first, then rename (atomic)
    QString tmpPath = cp + ".tmp";
    {
        QFile tmpFile(tmpPath);
        if (!tmpFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "ImageCache: cannot write" << tmpPath;
            return originalPath;
        }
        QImageWriter writer(&tmpFile, "JPEG");
        writer.setQuality(70);
        writer.setOptimizedWrite(true);
        if (!writer.write(img)) {
            qWarning() << "ImageCache: JPEG write failed" << cp << writer.errorString();
            tmpFile.remove();
            return originalPath;
        }
    }
    QFile::remove(cp);
    QFile::rename(tmpPath, cp);

    // Write sidecar metadata (original path) for prune
    QFile metaFile(metaPath(originalPath));
    if (metaFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        metaFile.write(originalPath.toUtf8());

    qDebug() << "ImageCache: created" << QFileInfo(cp).fileName()
             << "(" << QFileInfo(cp).size() / 1024 << "KB) from"
             << QFileInfo(originalPath).fileName()
             << "(" << QFileInfo(originalPath).size() / (1024*1024) << "MB)";

    return cp;
}

// Return compressed path if available, otherwise original
inline QString getCompressedOrOriginal(const QString &originalPath)
{
    if (originalPath.isEmpty()) return originalPath;
    QMutexLocker lock(&cacheMutex());
    QString cp = compressedPath(originalPath);
    if (QFileInfo::exists(cp)) return cp;
    return originalPath;
}

// Remove compressed copy when gallery item is deleted
inline void removeCompressed(const QString &originalPath)
{
    if (originalPath.isEmpty()) return;
    QMutexLocker lock(&cacheMutex());
    QFile::remove(compressedPath(originalPath));
    QFile::remove(metaPath(originalPath));
}

// Prune compressed copies whose originals no longer exist — O(n) via sidecar .meta files
inline void prune(const QSet<QString> &validOriginalPaths)
{
    QMutexLocker lock(&cacheMutex());
    QDir dir(compressedDir());
    QStringList metas = dir.entryList(QStringList() << "*.meta", QDir::Files);

    for (const QString &m : metas) {
        QFile f(dir.filePath(m));
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        QString origPath = QString::fromUtf8(f.readAll()).trimmed();
        f.close();

        if (!validOriginalPaths.contains(origPath)) {
            // Original removed from gallery — delete compressed + meta
            QString jpg = dir.filePath(QFileInfo(m).completeBaseName());
            QFile::remove(jpg);
            QFile::remove(dir.filePath(m));
        }
    }
}

} // namespace ImageCache
