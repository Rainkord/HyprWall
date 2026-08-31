#pragma once
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageWriter>
#include <QStandardPaths>
#include <QDebug>

namespace ImageCache {

inline QString compressedDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                  + "/hyprwall/compressed";
    QDir().mkpath(dir);
    return dir;
}

inline QString compressedPath(const QString &originalPath)
{
    QFileInfo fi(originalPath);
    return compressedDir() + "/" + fi.completeBaseName() + ".jpg";
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
    QString cp = compressedPath(originalPath);
    if (QFileInfo::exists(cp)) return cp;
    return originalPath;
}

// Remove compressed copy when gallery item is deleted
inline void removeCompressed(const QString &originalPath)
{
    if (originalPath.isEmpty()) return;
    QFile::remove(compressedPath(originalPath));
}

// Prune compressed copies whose originals no longer exist in gallery
inline void prune(const QSet<QString> &validOriginalPaths)
{
    QDir dir(compressedDir());
    QStringList files = dir.entryList(QStringList() << "*.jpg", QDir::Files);
    for (const QString &f : files) {
        // Reconstruct original path from compressed filename
        // We need to check if ANY valid path has this base name
        QString baseName = QFileInfo(f).completeBaseName();
        bool found = false;
        for (const QString &orig : validOriginalPaths) {
            if (QFileInfo(orig).completeBaseName() == baseName) {
                found = true;
                break;
            }
        }
        if (!found) {
            QFile::remove(dir.filePath(f));
        }
    }
}

} // namespace ImageCache
