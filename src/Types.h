#pragma once
#include <QString>

enum class FillMode {
    Fill,
    Fit,
    Stretch,
    Center,
    Tile,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    TopCenter,
    BottomCenter,
    CenterLeft,
    CenterRight
};

enum class WallpaperRotation {
    Normal,
    Portrait,
    UpsideDown,
    PortraitFlipped
};

struct MonitorInfo {
    QString name;
    int     x = 0;
    int     y = 0;
    int     width  = 1920;
    int     height = 1080;
    double  scale  = 1.0;
    int     refreshRate = 60;
    bool    connected = true;
};

struct WallpaperConfig {
    QString            monitorName;
    QString            filePath;
    FillMode           fillMode   = FillMode::Fill;
    WallpaperRotation  rotation   = WallpaperRotation::Normal;
    bool               audioEnabled = false;
    int                audioVolume  = 50;
};
