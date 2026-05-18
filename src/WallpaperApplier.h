#pragma once
#include "Types.h"
#include <QString>

class WallpaperApplier {
public:
    static bool    apply(const WallpaperConfig &cfg);
    static void    stopVideo(const QString &monitor);
    static void    toggleAudio(const QString &monitor);
    static bool    isVideoFile(const QString &path);
    static QString fillModeToHyprpaper(FillMode mode);
    // Если rotation != Normal — возвращает путь во временный файл, иначе пустую строку
    static QString prepareRotatedImage(const QString &src, WallpaperRotation rot);
};
