#pragma once
#include <QString>

// hyprpaper поддерживает только: contain | cover | tile | fill
// stretch/center/позиционирование — НЕ поддерживаются
enum class FillMode {
    Cover,    // заполнить (обрезает) — cover
    Contain,  // вписать целиком — contain
    Tile,     // плиткой — tile
    Fill,     // растянуть под экран — fill
};

// Поворот обоев (выполняется программно через QImage)
enum class WallpaperRotation {
    Normal,           // 0°
    Clockwise90,      // 90° по часовой
    Clockwise180,     // 180°
    Clockwise270,     // 270° по часовой
    FlipHorizontal,   // зеркало горизонтально
    FlipVertical,     // зеркало вертикально
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
    FillMode          fillMode   = FillMode::Cover;
    WallpaperRotation rotation   = WallpaperRotation::Normal;
    bool              audioEnabled = false;
    int               audioVolume  = 50;
};
