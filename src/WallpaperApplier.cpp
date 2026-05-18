#include "WallpaperApplier.h"
#include "ConfigManager.h"
#include <QProcess>
#include <QFileInfo>
#include <QStringList>
#include <QThread>
#include <QDebug>

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

static bool isSuccess(const QString &out)
{
    // hyprpaper 0.8.4 возвращает пустую строку при успехе, "ok"/"OK" в новых версиях
    return !out.toLower().contains("error") && !out.toLower().contains("bad");
}

static QString hyprpaperCmd(const QString &cmd, const QString &arg)
{
    QProcess p;
    p.start("hyprctl", {"hyprpaper", cmd, arg});
    p.waitForFinished(3000);
    QString out = p.readAllStandardOutput().trimmed();
    qDebug() << "hyprctl hyprpaper" << cmd << arg << "->" << (out.isEmpty() ? "(empty=ok)" : out);
    return out;
}

// Выполняет preload через hyprctl dispatch exec (обход бага v0.8.4)
static void doPreload(const QString &path)
{
    QProcess p;
    QString cmd = QString("hyprctl hyprpaper preload \"%1\"").arg(path);
    p.start("hyprctl", {"dispatch", "exec", cmd});
    p.waitForFinished(2000);
    qDebug() << "dispatch preload ->" << p.readAllStandardOutput().trimmed();
    QThread::msleep(400);
}

bool WallpaperApplier::apply(const WallpaperConfig &cfg)
{
    if (cfg.filePath.isEmpty()) {
        qWarning() << "apply: empty filePath for" << cfg.monitorName;
        return false;
    }

    if (isVideoFile(cfg.filePath)) {
        stopVideo(cfg.monitorName);
        QStringList args;
        args << "--mpv-options" << QString("loop%1").arg(
            cfg.audioEnabled ? QString(" --volume=%1").arg(cfg.audioVolume) : " --no-audio");
        args << cfg.monitorName << cfg.filePath;
        qDebug() << "mpvpaper" << args;
        return QProcess::startDetached("mpvpaper", args);
    }

    // Формируем аргумент wallpaper: MONITOR,[fill:]PATH
    QString fillStr = fillModeToHyprpaper(cfg.fillMode);
    // Пробуем сначала с fill prefix (работает в новых версиях)
    QString wallArgFill = QString("%1,%2:%3").arg(cfg.monitorName, fillStr, cfg.filePath);
    QString wallArgPlain = QString("%1,%2").arg(cfg.monitorName, cfg.filePath);

    // Первый попыток (fill prefix)
    QString out = hyprpaperCmd("wallpaper", wallArgFill);
    if (isSuccess(out)) return true;

    // Если ошибка содержит "bad path" — значит fill prefix не поддерживается, пробуем без него
    if (out.contains("bad path") || out.contains("bad")) {
        out = hyprpaperCmd("wallpaper", wallArgPlain);
        if (isSuccess(out)) return true;
    }

    // preload нужен — выполняем через dispatch
    doPreload(cfg.filePath);

    // Повторный попыток с fill prefix
    out = hyprpaperCmd("wallpaper", wallArgFill);
    if (isSuccess(out)) return true;

    // Последний шанс — без fill prefix
    out = hyprpaperCmd("wallpaper", wallArgPlain);
    return isSuccess(out);
}

void WallpaperApplier::stopVideo(const QString &monitor)
{
    QProcess::execute("pkill", {"-f", QString("mpvpaper.*%1").arg(monitor)});
}

void WallpaperApplier::toggleAudio(const QString &monitor)
{
    auto &cm = ConfigManager::instance();
    cm.load();
    WallpaperConfig cfg = cm.getConfig(monitor);
    cfg.audioEnabled    = !cfg.audioEnabled;
    cm.setConfig(monitor, cfg);
    cm.save();
    apply(cfg);
}
