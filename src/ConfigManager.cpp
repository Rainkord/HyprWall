#include "ConfigManager.h"
#include "WallpaperApplier.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QFile>
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
    QSettings s(configPath(), QSettings::IniFormat);

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
}

void ConfigManager::save()
{
    QSettings s(configPath(), QSettings::IniFormat);
    s.clear();

    for (auto it = m_configs.cbegin(); it != m_configs.cend(); ++it) {
        const WallpaperConfig &cfg = it.value();
        s.beginGroup(it.key());
        s.setValue("filePath",           cfg.filePath);
        s.setValue("fillMode",           static_cast<int>(cfg.fillMode));
        s.setValue("rotation",           static_cast<int>(cfg.rotation));
        s.setValue("audioEnabled",       cfg.audioEnabled);
        s.setValue("audioVolume",        cfg.audioVolume);
        s.setValue("slideshowEnabled",   cfg.slideshowEnabled);
        s.setValue("slideshowInterval",  cfg.slideshowInterval);
        s.setValue("slideshowMode",      cfg.slideshowMode);
        s.endGroup();
    }
}

QList<GalleryItem> ConfigManager::loadGallery() const
{
    QList<GalleryItem> items;
    const QStringList imageExts = {"jpg","jpeg","png","bmp","webp","tiff"};
    const QStringList videoExts = {"mp4","mkv","avi","webm","mov","gif","flv","wmv"};
    QDir d(galleryDir());
    d.setSorting(QDir::Time | QDir::Reversed);
    QStringList allExts;
    for (auto &e : imageExts) allExts << ("*." + e) << ("*." + e.toUpper());
    for (auto &e : videoExts) allExts << ("*." + e) << ("*." + e.toUpper());
    d.setNameFilters(allExts);
    for (const QFileInfo &fi : d.entryInfoList(QDir::Files)) {
        GalleryItem item;
        item.path    = fi.absoluteFilePath();
        item.isVideo = videoExts.contains(fi.suffix().toLower());
        items << item;
    }
    return items;
}

QList<GalleryItem> ConfigManager::addToGallery(const QStringList &paths)
{
    QString dest = galleryDir();
    QList<GalleryItem> added;
    for (const QString &src : paths) {
        QFileInfo fi(src);
        if (!fi.exists()) continue;
        QString dstPath = dest + "/" + fi.fileName();
        int n = 1;
        while (QFile::exists(dstPath))
            dstPath = dest + "/" + fi.baseName() + QString("_%1.").arg(n++) + fi.suffix();
        if (QFile::copy(src, dstPath)) {
            GalleryItem item;
            item.path    = dstPath;
            item.isVideo = WallpaperApplier::isVideoFile(dstPath);
            added << item;
        } else {
            qWarning() << "addToGallery: copy failed" << src << "->" << dstPath;
        }
    }
    return added;
}

void ConfigManager::removeFromGallery(const QString &path)
{
    QFile::remove(path);
}
