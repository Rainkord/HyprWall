#include "ServiceManager.h"
#include "ConfigManager.h"
#include "WallpaperApplier.h"
#include <QDebug>

void ServiceManager::applyAll(const QList<MonitorInfo> &monitors)
{
    ConfigManager &cm = ConfigManager::instance();
    cm.load();

    // If slideshow is enabled — apply a random gallery item to all monitors
    SlideshowConfig ss = cm.slideshowConfig();
    if (ss.enabled) {
        QList<GalleryItem> gallery = cm.loadGallery();
        if (!gallery.isEmpty()) {
            qDebug() << "[ServiceManager] slideshow enabled, applying random gallery item";
            WallpaperApplier::applySlideshowRandom(monitors, gallery);
            return;
        }
        qDebug() << "[ServiceManager] slideshow enabled but gallery is empty, falling back to per-monitor configs";
    }

    // Normal restore: apply each monitor’s last saved wallpaper
    for (const MonitorInfo &m : monitors) {
        WallpaperConfig cfg = cm.getConfig(m.name);
        if (!cfg.filePath.isEmpty()) {
            qDebug() << "[ServiceManager] restoring" << m.name << "->" << cfg.filePath;
            WallpaperApplier::apply(cfg);
        }
    }
}
