#include "ConfigManager.h"
#include "WallpaperApplier.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QSaveFile>
#include <QMutex>
#include <QDateTime>
#include <QDebug>

ConfigManager& ConfigManager::instance()
{
    static ConfigManager inst;
    return inst;
}

QString ConfigManager::configPath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/hyprwall";
    QDir().mkpath(dir);
    return dir + "/config.ini";
}

QString ConfigManager::galleryDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                  + "/hyprwall/gallery";
    QDir().mkpath(dir);
    return dir;
}

WallpaperConfig ConfigManager::getConfig(const QString &monitor) const
{
    if (m_configs.contains(monitor))
        return m_configs.value(monitor);
    WallpaperConfig cfg;
    cfg.monitorName = monitor;
    return cfg;
}

void ConfigManager::setConfig(const QString &monitor, const WallpaperConfig &cfg)
{
    m_configs[monitor] = cfg;
}

void ConfigManager::load()
{
    const QString path = configPath();
    if (!loadFromFile(path)) {
        // Main config corrupt — try backup
        const QString bakPath = path + ".bak";
        if (QFile::exists(bakPath)) {
            qWarning() << "ConfigManager: main config corrupt, restoring from .bak";
            QFile::remove(path);
            QFile::rename(bakPath, path);
            loadFromFile(path);
        }
    }
}

bool ConfigManager::loadFromFile(const QString &path)
{
    QSettings s(path, QSettings::IniFormat);
    if (s.status() != QSettings::NoError) return false;

    for (const QString &mon : s.childGroups()) {
        s.beginGroup(mon);
        WallpaperConfig cfg;
        cfg.monitorName         = mon;
        cfg.filePath            = s.value("filePath").toString();
        cfg.fillMode            = static_cast<FillMode>(s.value("fillMode", 0).toInt());
        cfg.rotation            = static_cast<WallpaperRotation>(s.value("rotation", 0).toInt());
        cfg.audioEnabled        = s.value("audioEnabled",       false).toBool();
        cfg.audioVolume         = s.value("audioVolume",        50).toInt();
        cfg.slideshowEnabled    = s.value("slideshowEnabled",   false).toBool();
        cfg.slideshowInterval   = s.value("slideshowInterval",  300).toInt();
        cfg.slideshowMode       = s.value("slideshowMode",      2).toInt();
        m_configs[mon]          = cfg;
        s.endGroup();
    }
    return true;
}

void ConfigManager::save()
{
    const QString path = configPath();
    const QString bakPath = path + ".bak";

    // Rotate: current → .bak (for crash recovery rollback)
    if (QFile::exists(path)) {
        QFile::remove(bakPath);
        QFile::rename(path, bakPath);
    }

    // Atomic write: QSaveFile guarantees no partial/corrupt config
    QSaveFile sf(path);
    if (!sf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "ConfigManager::save: cannot open" << path << sf.errorString();
        return;
    }
    QTextStream ts(&sf);
    for (auto it = m_configs.cbegin(); it != m_configs.cend(); ++it) {
        const WallpaperConfig &cfg = it.value();
        ts << "[" << it.key() << "]\n";
        ts << "filePath=" << cfg.filePath << "\n";
        ts << "fillMode=" << static_cast<int>(cfg.fillMode) << "\n";
        ts << "rotation=" << static_cast<int>(cfg.rotation) << "\n";
        ts << "audioEnabled=" << (cfg.audioEnabled ? "true" : "false") << "\n";
        ts << "audioVolume=" << cfg.audioVolume << "\n";
        ts << "slideshowEnabled=" << (cfg.slideshowEnabled ? "true" : "false") << "\n";
        ts << "slideshowInterval=" << cfg.slideshowInterval << "\n";
        ts << "slideshowMode=" << cfg.slideshowMode << "\n";
    }
    if (!sf.commit()) {
        qWarning() << "ConfigManager::save: commit failed" << sf.errorString();
        // Roll back from .bak if we renamed it
        if (QFile::exists(bakPath)) {
            QFile::remove(path);
            QFile::rename(bakPath, path);
        }
    }
}

QList<GalleryItem> ConfigManager::loadGallery() const
{
    QMutexLocker lock(&m_galleryMutex);

    // Check cache validity: only rescan if directory mtime changed
    QDir d(galleryDir());
    QDateTime currentMtime;
    const auto entries = d.entryInfoList(QDir::Files);
    if (!entries.isEmpty())
        currentMtime = entries.last().lastModified();  // most recent file change
    if (!m_galleryCache.isEmpty() && currentMtime == m_galleryDirMtime)
        return m_galleryCache;

    const QStringList imageExts = {"jpg","jpeg","png","bmp","webp","tiff"};
    const QStringList videoExts = {"mp4","mkv","avi","webm","mov","gif","flv","wmv"};
    d.setSorting(QDir::Time | QDir::Reversed);
    QStringList allExts;
    for (auto &e : imageExts) allExts << ("*." + e) << ("*." + e.toUpper());
    for (auto &e : videoExts) allExts << ("*." + e) << ("*." + e.toUpper());
    d.setNameFilters(allExts);

    m_galleryCache.clear();
    for (const QFileInfo &fi : entries) {
        GalleryItem item;
        item.path    = fi.absoluteFilePath();
        item.isVideo = videoExts.contains(fi.suffix().toLower());
        m_galleryCache << item;
    }
    m_galleryDirMtime = currentMtime;
    return m_galleryCache;
}

QList<GalleryItem> ConfigManager::addToGallery(const QStringList &paths)
{
    static constexpr qint64 MAX_FILE_SIZE = 500LL * 1024 * 1024; // 500 MB
    QString dest = galleryDir();
    QList<GalleryItem> added;
    for (const QString &src : paths) {
        QFileInfo fi(src);
        if (!fi.exists() || !fi.isFile()) continue;

        // Resolve symlinks to prevent symlink attack
        QString realPath = fi.isSymLink() ? fi.symLinkTarget() : fi.absoluteFilePath();
        QFileInfo realFi(realPath);
        if (!realFi.exists() || !realFi.isFile()) continue;

        // Size check
        if (realFi.size() > MAX_FILE_SIZE) {
            qWarning() << "addToGallery: file too large, skipped:" << src
                        << "(" << realFi.size() / (1024*1024) << "MB)";
            continue;
        }

        // Validate extension before copy
        QStringList validExts = {"jpg","jpeg","png","bmp","webp","gif",
                                 "mp4","mkv","avi","webm","mov"};
        if (!validExts.contains(realFi.suffix().toLower())) continue;

        QString dstPath = dest + "/" + realFi.fileName();
        int n = 1;
        while (QFile::exists(dstPath))
            dstPath = dest + "/" + realFi.completeBaseName() + QString("_%1.").arg(n++) + realFi.suffix();
        if (QFile::copy(realPath, dstPath)) {
            GalleryItem item;
            item.path    = dstPath;
            item.isVideo = WallpaperApplier::isVideoFile(dstPath);
            added << item;
        } else {
            qWarning() << "addToGallery: copy failed" << src << "->" << dstPath;
        }
    }
    invalidateGalleryCache();
    return added;
}

void ConfigManager::removeFromGallery(const QString &path)
{
    QFileInfo fi(path);
    QString gallery = QFileInfo(galleryDir()).absoluteFilePath();
    if (!fi.absoluteFilePath().startsWith(gallery)) {
        qWarning() << "removeFromGallery: refused, path outside gallery:" << path;
        return;
    }
    QFile::remove(path);
    invalidateGalleryCache();
}

void ConfigManager::invalidateGalleryCache() const
{
    QMutexLocker lock(&m_galleryMutex);
    m_galleryCache.clear();
    m_galleryDirMtime = QDateTime();
}
