#pragma once
#include "Types.h"
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>

class WallpaperApplier {
public:
    static bool        apply(const WallpaperConfig &cfg);
    static void        applyAll(const QMap<QString, WallpaperConfig> &configs);
    static void        stopVideo(const QString &monitor);
    static void        toggleAudio(const QString &monitor);
    static bool        isVideoFile(const QString &path);
    static QString     fillModeToHyprpaper(FillMode mode);
    static QString     prepareRotatedImage(const QString &src, WallpaperRotation rot);

    // Slideshow: pick a random gallery item and apply to all monitors
    // Videos are always applied without audio and with Cover/0deg
    static bool        applySlideshowRandom(const QList<MonitorInfo> &monitors,
                                            const QList<GalleryItem> &gallery);
};
