#pragma once
#include <QString>

enum class FillMode {
    Cover,
    Contain,
    Tile,
    Fill,
};

enum class WallpaperRotation {
    Normal,
    Clockwise90,
    Clockwise180,
    Clockwise270,
    FlipHorizontal,
    FlipVertical,
};

enum class WallpaperMode {
    Static,    // single file
    Slideshow, // folder with interval
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
    QString           filePath;       // for Static mode
    QString           folderPath;     // for Slideshow mode
    WallpaperMode     mode           = WallpaperMode::Static;
    int               slideshowSecs  = 600; // seconds between changes
    FillMode          fillMode       = FillMode::Cover;
    WallpaperRotation rotation       = WallpaperRotation::Normal;
    bool              audioEnabled   = false;
    int               audioVolume    = 50;
};
