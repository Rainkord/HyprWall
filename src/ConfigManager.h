#pragma once
#include <QList>
#include <QMap>
#include <QString>
#include "Types.h"

class ConfigManager {
public:
    static ConfigManager& instance();

    void load();
    void save();

    // Per-monitor wallpaper config
    WallpaperConfig getConfig(const QString &monitor) const;
    void            setConfig(const QString &monitor, const WallpaperConfig &cfg);
    const QMap<QString, WallpaperConfig>& configs() const { return m_configs; }

    // Shared wallpaper gallery (paths stored in ~/.local/share/hyprwall/gallery/)
    static QString galleryDir();
    QList<GalleryItem> loadGallery() const;
    QList<GalleryItem> addToGallery(const QStringList &paths);
    void removeFromGallery(const QString &path);

private:
    ConfigManager() = default;
    static QString configPath();

    QMap<QString, WallpaperConfig> m_configs;
};
