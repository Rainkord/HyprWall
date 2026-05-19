#pragma once
#include "Types.h"
#include <QString>
#include <QStringList>

class WallpaperApplier {
public:
    static bool        apply(const WallpaperConfig &cfg);
    static void        stopVideo(const QString &monitor);
    static void        toggleAudio(const QString &monitor);
    static bool        isVideoFile(const QString &path);
    static QString     fillModeToHyprpaper(FillMode mode);
    static QString     prepareRotatedImage(const QString &src, WallpaperRotation rot);

    // Slideshow helpers (pure C++, no external scripts)
    static QStringList scanImageFolder(const QString &folder);
    static bool        applyRandomFromFolder(const QString &monitor,
                                             const QString &folder,
                                             FillMode fill,
                                             WallpaperRotation rot);
};
