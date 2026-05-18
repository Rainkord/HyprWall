#include "WallpaperApplier.h"
#include "ConfigManager.h"
#include <QProcess>
#include <QFileInfo>
#include <QStringList>
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
    Q_UNUSED(mode)
    return "";
}

// Отправляет команду в hyprpaper через hyprctl и возвращает stdout
static QString hyprpaperCmd(const QString &cmd, const QString &arg)
{
    QProcess p;
    p.start("hyprctl", {"hyprpaper", cmd, arg});
    p.waitForFinished(3000);
    QString out = p.readAllStandardOutput().trimmed();
    qDebug() << "hyprctl hyprpaper" << cmd << arg << "->" << out;
    return out;
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

    // hyprpaper 0.8.4: preload через hyprctl сломан, wallpaper работает
    // Сначала пробуем wallpaper без preload
    QString wallArg = QString("%1,%2").arg(cfg.monitorName, cfg.filePath);
    QString out = hyprpaperCmd("wallpaper", wallArg);

    if (out == "ok" || out == "OK") return true;

    // Если не сработало — значит нужен preload. Пробуем через hyprctl dispatch exec
    // чтобы preload выполнился уже на стороне hyprpaper (обход бага)
    qDebug() << "wallpaper failed, trying preload via dispatch...";

    // hyprctl dispatch exec — запускает команду в контексте Hyprland
    {
        QProcess p;
        QString preloadCmd = QString("hyprctl hyprpaper preload \"%1\"").arg(cfg.filePath);
        p.start("hyprctl", {"dispatch", "exec", preloadCmd});
        p.waitForFinished(2000);
        qDebug() << "dispatch preload ->" << p.readAllStandardOutput().trimmed();
    }

    // Ждём пока preload отработает
    QThread::msleep(500);

    // Повторный вызов wallpaper
    out = hyprpaperCmd("wallpaper", wallArg);
    return (out == "ok" || out == "OK");
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
