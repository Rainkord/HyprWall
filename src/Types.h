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
    QString description;
    int     x = 0;
    int     y = 0;
    int     width  = 1920;   // логические пиксели (с учётом transform)
    int     height = 1080;
    double  scale  = 1.0;
    int     refreshRate = 60;
    int     transform = 0;   // 0=normal,1=90,2=180,3=270,4=flipped,...
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
