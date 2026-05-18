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

// hyprpaper fill modes: https://wiki.hyprland.org/Hypr-Ecosystem/hyprpaper/
QString WallpaperApplier::fillModeToHyprpaper(FillMode mode)
{
    switch (mode) {
        case FillMode::Fill:         return "fill";
        case FillMode::Fit:          return "contain";
        case FillMode::Stretch:      return "stretch";
        case FillMode::Center:       return "center";
        case FillMode::Tile:         return "tile";
        // остальные — позиционные, hyprpaper их не поддерживает напрямую
        // используем center как fallback
        default:                     return "center";
    }
}

bool WallpaperApplier::apply(const WallpaperConfig &cfg)
{
    if (cfg.filePath.isEmpty()) return false;

    if (isVideoFile(cfg.filePath)) {
        // убиваем предыдущий mpvpaper на этом мониторе
        stopVideo(cfg.monitorName);

        // mpvpaper [options] <output> <file>
        // правильный порядок аргументов: сначала флаги, потом монитор и файл
        QStringList args;
        // строим mpv-options
        QStringList mpvOpts;
        mpvOpts << "loop";
        if (!cfg.audioEnabled)
            mpvOpts << "--no-audio";
        else
            mpvOpts << QString("--volume=%1").arg(cfg.audioVolume);

        args << "--mpv-options" << mpvOpts.join(" ");
        args << cfg.monitorName << cfg.filePath;

        qDebug() << "mpvpaper" << args;
        bool ok = QProcess::startDetached("mpvpaper", args);
        if (!ok) qWarning() << "Failed to start mpvpaper";
        return ok;
    } else {
        // --- hyprpaper ---
        // 1. Проверяем что hyprpaper запущен (запускаем если нет)
        {
            QProcess check;
            check.start("pgrep", {"-x", "hyprpaper"});
            check.waitForFinished(1000);
            if (check.readAllStandardOutput().trimmed().isEmpty()) {
                qDebug() << "Starting hyprpaper daemon...";
                QProcess::startDetached("hyprpaper", {});
                // даём секунду подняться
                QProcess::execute("sleep", {"1"});
            }
        }

        // 2. preload
        {
            QProcess p;
            p.start("hyprctl", {"hyprpaper", "preload", cfg.filePath});
            p.waitForFinished(3000);
            QString out = p.readAllStandardOutput().trimmed();
            QString err = p.readAllStandardError().trimmed();
            qDebug() << "preload out:" << out << "err:" << err;
        }

        // 3. wallpaper — формат: "<monitor>,<path>" (без fill prefix в старых версиях)
        //    В новых hyprpaper: "<monitor>,<fill>:<path>" или "<monitor>,<path>"
        //    Пробуем сначала с fill-prefix, это рабочий формат
        QString fillStr = fillModeToHyprpaper(cfg.fillMode);
        // Полный формат: MONITOR,fill:PATH
        QString wallArg = QString("%1,%2:%3").arg(cfg.monitorName, fillStr, cfg.filePath);
        qDebug() << "hyprctl hyprpaper wallpaper" << wallArg;

        QProcess p2;
        p2.start("hyprctl", {"hyprpaper", "wallpaper", wallArg});
        p2.waitForFinished(3000);
        QString out2 = p2.readAllStandardOutput().trimmed();
        QString err2 = p2.readAllStandardError().trimmed();
        qDebug() << "wallpaper out:" << out2 << "err:" << err2;

        // Если вернулось не "ok" — пробуем без fill prefix (старый синтаксис)
        if (out2 != "ok" && out2 != "OK") {
            QString wallArgSimple = QString("%1,%2").arg(cfg.monitorName, cfg.filePath);
            qDebug() << "Retrying without fill prefix:" << wallArgSimple;
            QProcess p3;
            p3.start("hyprctl", {"hyprpaper", "wallpaper", wallArgSimple});
            p3.waitForFinished(3000);
            QString out3 = p3.readAllStandardOutput().trimmed();
            qDebug() << "wallpaper retry out:" << out3;
            return (out3 == "ok" || out3 == "OK");
        }
        return true;
    }
}

void WallpaperApplier::stopVideo(const QString &monitor)
{
    // ищем процесс mpvpaper с именем монитора в аргументах
    QProcess::execute("pkill", {"-f", QString("mpvpaper.*%1").arg(monitor)});
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
