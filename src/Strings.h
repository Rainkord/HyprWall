#pragma once
#include <QString>
#include <QStringList>

struct Strings {
    QString windowTitle;
    QString noMonitors;
    QString groupTitle;
    QString audioCheck;
    QString volumeLabel;
    QString rotLabel;
    QString bindPrefix;
    QString langLabel;
    QString autostartLabel;
    QString orientLandscape;
    QString orientPortrait90;
    QString orientLandscape180;
    QString orientPortrait270;
    QStringList imgFillModes;
    QStringList imgRotModes;
    QStringList vidRotModes;
    // Gallery
    QString galleryTitle;
    QString galleryAddBtn;
    QString galleryEmptyHint;
    QString galleryRemoveTooltip;
    // Slideshow
    QString slideshowLabel;
    QString slideshowIntervalLabel;
    QString slideshowMinLabel;
    QString slideshowModeLabel;
    QStringList slideshowModes;
    // Interval preset labels
    QStringList intervalLabels;
    // Lock screen
    QString lockScreenGroupTitle;
    // Tabs
    QString tabDesktop;
    QString tabLockScreen;
    QString sameWallpaperLabel;
};

inline Strings stringsEN()
{
    Strings s;
    s.windowTitle      = "HyprWall";
    s.noMonitors       = "No monitors detected";
    s.groupTitle       = "WALLPAPER SETTINGS";
    s.audioCheck       = "Enable audio";
    s.volumeLabel      = "Volume:";
    s.rotLabel         = "Rotation:";
    s.bindPrefix       = "Hyprland audio toggle bind:";
    s.langLabel        = "Lang:";
    s.autostartLabel   = "Autostart:";
    s.orientLandscape    = "Landscape";
    s.orientPortrait90   = "Portrait 90\u00b0";
    s.orientLandscape180 = "Landscape 180\u00b0";
    s.orientPortrait270  = "Portrait 270\u00b0";
    s.imgFillModes = { "Cover", "Contain" };
    s.imgRotModes  = { "0\u00b0", "90\u00b0", "180\u00b0", "270\u00b0" };
    s.vidRotModes  = { "0\u00b0", "90\u00b0", "180\u00b0", "270\u00b0" };
    // Gallery
    s.galleryTitle         = "WALLPAPER GALLERY";
    s.galleryAddBtn        = "+ Add";
    s.galleryEmptyHint     = "Click \u201c+ Add\u201d to import wallpapers";
    s.galleryRemoveTooltip = "Remove from gallery";
    // Slideshow
    s.slideshowLabel         = "Slideshow:";
    s.slideshowIntervalLabel = "Change every";
    s.slideshowMinLabel      = "min";
    s.slideshowModeLabel     = "Media type:";
    s.slideshowModes = {
        "Photos only",
        "Videos only",
        "Photos + Videos"
    };
    s.intervalLabels = { "1 min", "5 min", "10 min", "15 min", "30 min", "1 hour" };
    // Lock screen
    s.lockScreenGroupTitle = "LOCK SCREEN WALLPAPER";
    // Tabs
    s.tabDesktop       = "Desktop";
    s.tabLockScreen    = "Lock Screen";
    s.sameWallpaperLabel = "Sync with lock screen:";
    return s;
}

inline Strings stringsRU()
{
    Strings s;
    s.windowTitle      = "HyprWall";
    s.noMonitors       = "\u041c\u043e\u043d\u0438\u0442\u043e\u0440\u044b \u043d\u0435 \u043e\u0431\u043d\u0430\u0440\u0443\u0436\u0435\u043d\u044b";
    s.groupTitle       = "\u041d\u0410\u0421\u0422\u0420\u041e\u0419\u041a\u0410 \u041e\u0411\u041e\u0415\u0412";
    s.audioCheck       = "\u0412\u043a\u043b\u044e\u0447\u0438\u0442\u044c \u0437\u0432\u0443\u043a";
    s.volumeLabel      = "\u0413\u0440\u043e\u043c\u043a\u043e\u0441\u0442\u044c:";
    s.rotLabel         = "\u041f\u043e\u0432\u043e\u0440\u043e\u0442:";
    s.bindPrefix       = "\u0411\u0438\u043d\u0434 Hyprland \u0434\u043b\u044f \u0437\u0432\u0443\u043a\u0430 \u0432\u0438\u0434\u0435\u043e:";
    s.langLabel        = "\u042f\u0437\u044b\u043a:";
    s.autostartLabel   = "\u0410\u0432\u0442\u043e\u0441\u0442\u0430\u0440\u0442:";
    s.orientLandscape    = "\u0413\u043e\u0440\u0438\u0437\u043e\u043d\u0442\u0430\u043b\u044c";
    s.orientPortrait90   = "\u041f\u043e\u0440\u0442\u0440\u0435\u0442 90\u00b0";
    s.orientLandscape180 = "\u0413\u043e\u0440\u0438\u0437\u043e\u043d\u0442\u0430\u043b\u044c 180\u00b0";
    s.orientPortrait270  = "\u041f\u043e\u0440\u0442\u0440\u0435\u0442 270\u00b0";
    s.imgFillModes = {
        "\u0417\u0430\u043f\u043e\u043b\u043d\u0435\u043d\u0438\u0435",
        "\u0412\u043f\u0438\u0441\u0430\u0442\u044c"
    };
    s.imgRotModes  = { "0\u00b0", "90\u00b0", "180\u00b0", "270\u00b0" };
    s.vidRotModes  = { "0\u00b0", "90\u00b0", "180\u00b0", "270\u00b0" };
    // Gallery
    s.galleryTitle         = "\u0413\u0410\u041b\u0415\u0420\u0415\u042f \u041e\u0411\u041e\u0415\u0412";
    s.galleryAddBtn        = "+ \u0414\u043e\u0431\u0430\u0432\u0438\u0442\u044c";
    s.galleryEmptyHint     = "\u041d\u0430\u0436\u043c\u0438\u0442\u0435 \u00ab+ \u0414\u043e\u0431\u0430\u0432\u0438\u0442\u044c\u00bb \u0447\u0442\u043e\u0431\u044b \u0438\u043c\u043f\u043e\u0440\u0442\u0438\u0440\u043e\u0432\u0430\u0442\u044c \u043e\u0431\u043e\u0438";
    s.galleryRemoveTooltip = "\u0423\u0434\u0430\u043b\u0438\u0442\u044c \u0438\u0437 \u0433\u0430\u043b\u0435\u0440\u0435\u0438";
    // Slideshow
    s.slideshowLabel         = "\u0421\u043b\u0430\u0439\u0434-\u0448\u043e\u0443:";
    s.slideshowIntervalLabel = "\u041c\u0435\u043d\u044f\u0442\u044c \u043a\u0430\u0436\u0434\u044b\u0435";
    s.slideshowMinLabel      = "\u043c\u0438\u043d.";
    s.slideshowModeLabel     = "\u0422\u0438\u043f \u043c\u0435\u0434\u0438\u0430:";
    s.slideshowModes = {
        "\u0422\u043e\u043b\u044c\u043a\u043e \u0444\u043e\u0442\u043e",
        "\u0422\u043e\u043b\u044c\u043a\u043e \u0432\u0438\u0434\u0435\u043e",
        "\u0424\u043e\u0442\u043e + \u0432\u0438\u0434\u0435\u043e"
    };
    s.intervalLabels = {
        "1 \u043c\u0438\u043d",
        "5 \u043c\u0438\u043d",
        "10 \u043c\u0438\u043d",
        "15 \u043c\u0438\u043d",
        "30 \u043c\u0438\u043d",
        "1 \u0447\u0430\u0441"
    };
    // Lock screen
    s.lockScreenGroupTitle = "\u041e\u0411\u041e\u0418 \u0417\u0410\u0411\u041b\u041e\u041a\u0418\u0420\u041e\u0412\u041a\u0418";
    // Tabs
    s.tabDesktop       = "\u0420\u0430\u0431\u043e\u0447\u0438\u0439 \u0441\u0442\u043e\u043b";
    s.tabLockScreen    = "\u042d\u043a\u0440\u0430\u043d \u0431\u043b\u043e\u043a\u0438\u0440\u043e\u0432\u043a\u0438";
    s.sameWallpaperLabel = "\u0421\u0438\u043d\u0445\u0440\u043e\u043d\u0438\u0437\u0438\u0440\u043e\u0432\u0430\u0442\u044c \u043e\u0431\u043e\u0438 \u0441 \u044d\u043a\u0440\u0430\u043d\u043e\u043c \u0431\u043b\u043e\u043a\u0438\u0440\u043e\u0432\u043a\u0438:";
    return s;
}
