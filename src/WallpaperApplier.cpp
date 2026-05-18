#include "WallpaperApplier.h"
#include "ConfigManager.h"
#include <QProcess>
#include <QFileInfo>
#include <QStringList>

static const QStringList VIDEO_EXTS = {
    "mp4","mkv","avi","webm","mov","gif","flv","wmv"
};

bool WallpaperApplier::isVideoFile(const QString &path)
{
    return VIDEO_EXTS.contains(QFileInfo(path).suffix().toLower());
}

QString WallpaperApplier::fillModeToHyprpaper(FillMode mode)
{
    switch (mode) {
        case FillMode::Fill:    return "fill";
        case FillMode::Fit:     return "contain";
        case FillMode::Stretch: return "stretch";
        case FillMode::Center:  return "center";
        case FillMode::Tile:    return "tile";
        default:                return "fill";
    }
}

bool WallpaperApplier::apply(const WallpaperConfig &cfg)
{
    if (cfg.filePath.isEmpty()) return false;

    if (isVideoFile(cfg.filePath)) {
        stopVideo(cfg.monitorName);
        QStringList args;
        args << cfg.monitorName << cfg.filePath;
        QString mpvOpts = "loop";
        if (!cfg.audioEnabled) mpvOpts += " --no-audio";
        else mpvOpts += QString(" --volume=%1").arg(cfg.audioVolume);
        args << "--mpv-options" << mpvOpts;
        return QProcess::startDetached("mpvpaper", args);
    } else {
        QProcess p1;
        p1.start("hyprctl", {"hyprpaper", "preload", cfg.filePath});
        p1.waitForFinished(2000);

        QString fillStr = fillModeToHyprpaper(cfg.fillMode);
        QString wallArg = QString("%1,%2:%3").arg(cfg.monitorName, fillStr, cfg.filePath);
        QProcess p2;
        p2.start("hyprctl", {"hyprpaper", "wallpaper", wallArg});
        p2.waitForFinished(2000);
        return p2.exitCode() == 0;
    }
}

void WallpaperApplier::stopVideo(const QString &monitor)
{
    QProcess::execute("pkill", {"-f", QString("mpvpaper %1").arg(monitor)});
}

void WallpaperApplier::toggleAudio(const QString &monitor)
{
    auto &cm  = ConfigManager::instance();
    cm.load();
    WallpaperConfig cfg = cm.getConfig(monitor);
    cfg.audioEnabled    = !cfg.audioEnabled;
    cm.setConfig(monitor, cfg);
    cm.save();
    apply(cfg);
}
