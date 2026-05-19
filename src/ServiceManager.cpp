#include "ServiceManager.h"
#include "ConfigManager.h"
#include "WallpaperApplier.h"
#include <QTimer>
#include <QCoreApplication>
#include <QDebug>

void ServiceManager::applyAll(const QList<MonitorInfo> &/*monitors*/)
{
    ConfigManager &cm = ConfigManager::instance();
    cm.load();

    // Apply saved static wallpapers for all monitors
    WallpaperApplier::applyAll(cm.configs());

    // Start per-monitor slideshow timers for monitors that have it enabled
    const auto configs = cm.configs();
    for (auto it = configs.cbegin(); it != configs.cend(); ++it) {
        const WallpaperConfig &cfg = it.value();
        if (!cfg.slideshowEnabled || cfg.slideshowInterval <= 0) continue;
        const QString mon = it.key();
        const int mode    = cfg.slideshowMode;
        const int msecs   = cfg.slideshowInterval * 1000;
        QTimer *t = new QTimer(QCoreApplication::instance());
        t->setInterval(msecs);
        QObject::connect(t, &QTimer::timeout, [mon, mode](){
            QList<GalleryItem> gallery = ConfigManager::instance().loadGallery();
            WallpaperApplier::applySlideshowTick(mon, gallery, mode);
        });
        // First tick immediately
        QTimer::singleShot(0, [mon, mode](){
            QList<GalleryItem> gallery = ConfigManager::instance().loadGallery();
            WallpaperApplier::applySlideshowTick(mon, gallery, mode);
        });
        t->start();
        qDebug() << "Slideshow started for" << mon << "every" << cfg.slideshowInterval << "sec";
    }
}
