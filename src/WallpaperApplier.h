#pragma once
#include "Types.h"
#include <QString>
#include <QList>
#include <QMap>

class WallpaperApplier {
public:
    static bool    apply(const WallpaperConfig &cfg);
    static void    applyAll(const QMap<QString, WallpaperConfig> &configs);
    static bool    applySlideshowTick(const QString &monitor,
                                      const QList<GalleryItem> &gallery,
                                      int mode);  // 0=photos,1=videos,2=both
    static void    stopVideo(const QString &monitor);
    static void    toggleAudio(const QString &monitor);
    static bool    isVideoFile(const QString &path);
    static bool    isGifFile(const QString &path);
    static QString fillModeToHyprpaper(FillMode mode);
    static QString prepareRotatedImage(const QString &src, WallpaperRotation rot);
};
