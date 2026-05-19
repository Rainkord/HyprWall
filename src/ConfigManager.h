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
    // Add files to gallery (copies them into galleryDir)
    QList<GalleryItem> addToGallery(const QStringList &paths);
    // Remove one item from gallery (deletes the file)
    void removeFromGallery(const QString &path);

    // Slideshow
    SlideshowConfig slideshowConfig() const { return m_slideshow; }
    void setSlideshowConfig(const SlideshowConfig &s) { m_slideshow = s; }

private:
    ConfigManager() = default;
    static QString configPath();

    QMap<QString, WallpaperConfig> m_configs;
    SlideshowConfig                m_slideshow;
};
