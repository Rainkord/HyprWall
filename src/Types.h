#pragma once
#include <QString>
#include <QStringList>

enum class FillMode {
    Cover,
    Contain,
};

enum class WallpaperRotation {
    Normal,
    Clockwise90,
    Clockwise180,
    Clockwise270,
};

struct MonitorInfo {
    QString name;
    QString description;
    int     x = 0;
    int     y = 0;
    int     width  = 1920;
    int     height = 1080;
    double  scale  = 1.0;
    int     refreshRate = 60;
    int     transform = 0;
    bool    connected = true;
};

struct WallpaperConfig {
    QString           monitorName;
    QString           filePath;
    FillMode          fillMode      = FillMode::Cover;
    WallpaperRotation rotation      = WallpaperRotation::Normal;
    bool              audioEnabled  = false;
    int               audioVolume   = 50;
    // Per-monitor slideshow
    bool              slideshowEnabled  = false;
    int               slideshowInterval = 300;   // seconds
    int               slideshowMode     = 2;     // 0=photos, 1=videos, 2=both
};

struct GalleryItem {
    QString path;     // absolute path inside gallery store
    bool    isVideo = false;
};
