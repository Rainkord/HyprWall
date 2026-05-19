#pragma once
#include <QString>
#include <QStringList>

struct Strings {
    QString windowTitle;
    QString noMonitors;
    QString monitorLabel;
    QString groupTitle;
    QString fileLabel;
    QString browseBtn;
    QString audioCheck;
    QString volumeLabel;
    QString fillLabel;
    QString rotLabel;
    QString applyBtn;
    QString bindPrefix;
    QString errTitle;
    QString errBody;
    QString langLabel;
    QString autostartLabel;
    QString autostartEnable;
    QString autostartDisable;
    // Orientation strings
    QString orientLandscape;
    QString orientPortrait90;
    QString orientLandscape180;
    QString orientPortrait270;
    // Fill modes
    QStringList imgFillModes;
    QStringList vidFillModes;
    // Rotation modes
    QStringList imgRotModes;
    QStringList vidRotModes;
};

inline Strings stringsEN()
{
    Strings s;
    s.windowTitle      = "HyprWall";
    s.noMonitors       = "No monitors detected";
    s.monitorLabel     = "Monitor:";
    s.groupTitle       = "WALLPAPER SETTINGS";
    s.fileLabel        = "File:";
    s.browseBtn        = "Browse";
    s.audioCheck       = "Enable audio";
    s.volumeLabel      = "Volume:";
    s.fillLabel        = "Fill:";
    s.rotLabel         = "Rotation:";
    s.applyBtn         = "Apply";
    s.bindPrefix       = "Hyprland audio toggle bind:";
    s.errTitle         = "Error";
    s.errBody          = "Failed to apply wallpaper. Is hyprpaper running?";
    s.langLabel        = "Lang:";
    s.autostartLabel   = "Autostart:";
    s.autostartEnable  = "Enable";
    s.autostartDisable = "Disable";
    s.orientLandscape    = "Landscape";
    s.orientPortrait90   = "Portrait 90\u00b0";
    s.orientLandscape180 = "Landscape 180\u00b0";
    s.orientPortrait270  = "Portrait 270\u00b0";
    s.imgFillModes = { "Cover", "Contain" };
    s.vidFillModes = { "Cover (pan)", "Contain" };
    s.imgRotModes  = { "0\u00b0", "90\u00b0", "180\u00b0", "270\u00b0" };
    s.vidRotModes  = { "0\u00b0", "90\u00b0", "180\u00b0", "270\u00b0" };
    return s;
}

inline Strings stringsRU()
{
    Strings s;
    s.windowTitle      = "HyprWall";
    s.noMonitors       = "\u041c\u043e\u043d\u0438\u0442\u043e\u0440\u044b \u043d\u0435 \u043e\u0431\u043d\u0430\u0440\u0443\u0436\u0435\u043d\u044b";
    s.monitorLabel     = "\u041c\u043e\u043d\u0438\u0442\u043e\u0440:";
    s.groupTitle       = "\u041d\u0410\u0421\u0422\u0420\u041e\u0419\u041a\u0418 \u041e\u0411\u041e\u0418";
    s.fileLabel        = "\u0424\u0430\u0439\u043b:";
    s.browseBtn        = "\u041e\u0431\u0437\u043e\u0440";
    s.audioCheck       = "\u0412\u043a\u043b\u044e\u0447\u0438\u0442\u044c \u0430\u0443\u0434\u0438\u043e";
    s.volumeLabel      = "\u0413\u0440\u043e\u043c\u043a\u043e\u0441\u0442\u044c:";
    s.fillLabel        = "\u0417\u0430\u043f\u043e\u043b\u043d\u0435\u043d\u0438\u0435:";
    s.rotLabel         = "\u041f\u043e\u0432\u043e\u0440\u043e\u0442:";
    s.applyBtn         = "\u041f\u0440\u0438\u043c\u0435\u043d\u0438\u0442\u044c";
    s.bindPrefix       = "\u0411\u0438\u043d\u0434 \u0430\u0443\u0434\u0438\u043e (Hyprland):";
    s.errTitle         = "\u041e\u0448\u0438\u0431\u043a\u0430";
    s.errBody          = "\u041d\u0435 \u0443\u0434\u0430\u043b\u043e\u0441\u044c \u043f\u0440\u0438\u043c\u0435\u043d\u0438\u0442\u044c \u043e\u0431\u043e\u0438. \u0417\u0430\u043f\u0443\u0449\u0435\u043d \u043b\u0438 hyprpaper?";
    s.langLabel        = "\u042f\u0437\u044b\u043a:";
    s.autostartLabel   = "\u0410\u0432\u0442\u043e\u0437\u0430\u043f\u0443\u0441\u043a:";
    s.autostartEnable  = "\u0412\u043a\u043b\u044e\u0447\u0438\u0442\u044c";
    s.autostartDisable = "\u0412\u044b\u043a\u043b\u044e\u0447\u0438\u0442\u044c";
    s.orientLandscape    = "\u0413\u043e\u0440\u0438\u0437\u043e\u043d\u0442\u0430\u043b\u044c";
    s.orientPortrait90   = "\u041f\u043e\u0440\u0442\u0440\u0435\u0442 90\u00b0";
    s.orientLandscape180 = "\u0413\u043e\u0440\u0438\u0437\u043e\u043d\u0442\u0430\u043b\u044c 180\u00b0";
    s.orientPortrait270  = "\u041f\u043e\u0440\u0442\u0440\u0435\u0442 270\u00b0";
    s.imgFillModes = { "\u0417\u0430\u043f\u043e\u043b\u043d\u0438\u0442\u044c", "\u0412\u043f\u0438\u0441\u0430\u0442\u044c" };
    s.vidFillModes = { "\u0417\u0430\u043f\u043e\u043b\u043d\u0438\u0442\u044c", "\u0412\u043f\u0438\u0441\u0430\u0442\u044c" };
    s.imgRotModes  = { "0\u00b0", "90\u00b0", "180\u00b0", "270\u00b0" };
    s.vidRotModes  = s.imgRotModes;
    return s;
}
