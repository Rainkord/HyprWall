#pragma once
#include <QList>
#include <QMap>
#include <QString>
#include <QMutex>
#include <QDateTime>
#include "Types.h"

class QFileSystemWatcher;

class ConfigManager {
public:
    static ConfigManager& instance();

    void load();
    void save();

    // Per-monitor wallpaper config
    WallpaperConfig getConfig(const QString &monitor) const;
    void            setConfig(const QString &monitor, const WallpaperConfig &cfg);
    const QMap<QString, WallpaperConfig>& configs() const { return m_configs; }

    // Lock screen (hyprlock) per-monitor config
    WallpaperConfig getHyprlockConfig(const QString &monitor) const;
    void            setHyprlockConfig(const QString &monitor, const WallpaperConfig &cfg);
    const QMap<QString, WallpaperConfig>& hyprlockConfigs() const { return m_hyprlockConfigs; }
    void            writeHyprlockConf();

    // Shared wallpaper gallery (paths stored in ~/.local/share/hyprwall/gallery/)
    static QString galleryDir();
    QList<GalleryItem> loadGallery() const;
    QList<GalleryItem> addToGallery(const QStringList &paths);
    void removeFromGallery(const QString &path);

    // General settings
    QVariant getSetting(const QString &key, const QVariant &def = {}) const;
    void     setSetting(const QString &key, const QVariant &val);

    // Language — stored in a separate file to survive config.ini rewrites
    int  loadLanguage();
    void saveLanguage(int langIndex);

    // Same wallpaper toggle — separate file like language
    bool loadSameWallpaper();
    void saveSameWallpaper(bool on);

private:
    ConfigManager() = default;
    static QString configPath();
    bool loadFromFile(const QString &path);

    QMap<QString, WallpaperConfig> m_configs;
    QMap<QString, WallpaperConfig> m_hyprlockConfigs;
    QMap<QString, QVariant> m_general;

    // Gallery cache (mutable — const loadGallery() writes to cache)
    mutable QMutex m_galleryMutex;
    mutable QList<GalleryItem> m_galleryCache;
    mutable QDateTime m_galleryDirMtime;
    void invalidateGalleryCache() const;
};
