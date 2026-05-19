#include "WallpaperApplier.h"
#include "ConfigManager.h"
#include <QProcess>
#include <QFileInfo>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>
#include <QThread>
#include <QImage>
#include <QTransform>
#include <QDebug>

static const QStringList VIDEO_EXTS = {
    "mp4","mkv","avi","webm","mov","flv","wmv"
};

bool WallpaperApplier::isVideoFile(const QString &path)
{
    return VIDEO_EXTS.contains(QFileInfo(path).suffix().toLower());
}

QString WallpaperApplier::fillModeToHyprpaper(FillMode mode)
{
    switch (mode) {
        case FillMode::Cover:   return "cover";
        case FillMode::Contain: return "contain";
        case FillMode::Tile:    return "tile";
        case FillMode::Fill:    return "fill";
        default:                return "cover";
    }
}

static QString mpvOptions(int fillIdx, int rotIdx, bool audio, int volume)
{
    QStringList opts;
    opts << "--loop";
    switch (fillIdx) {
        case 0: opts << "--panscan=1.0";    break;
        case 1: opts << "--keepaspect=yes"; break;
        case 2: opts << "--keepaspect=no";  break;
        default: opts << "--keepaspect=yes"; break;
    }
    switch (rotIdx) {
        case 1: opts << "--video-rotate=90";  break;
        case 2: opts << "--video-rotate=180"; break;
        case 3: opts << "--video-rotate=270"; break;
        case 4: opts << "--vf=hflip";         break;
        case 5: opts << "--vf=vflip";         break;
        default: break;
    }
    if (!audio)
        opts << "--no-audio";
    else
        opts << QString("--volume=%1").arg(volume);
    return opts.join(" ");
}

QString WallpaperApplier::prepareRotatedImage(const QString &src, WallpaperRotation rot)
{
    if (rot == WallpaperRotation::Normal) return QString();
    QImage img(src);
    if (img.isNull()) { qWarning() << "prepareRotatedImage: cannot load" << src; return QString(); }
    QImage result;
    switch (rot) {
        case WallpaperRotation::Clockwise90: {
            QTransform t; t.rotate(90);
            result = img.transformed(t, Qt::SmoothTransformation); break;
        }
        case WallpaperRotation::Clockwise180: {
            QTransform t; t.rotate(180);
            result = img.transformed(t, Qt::SmoothTransformation); break;
        }
        case WallpaperRotation::Clockwise270: {
            QTransform t; t.rotate(270);
            result = img.transformed(t, Qt::SmoothTransformation); break;
        }
        case WallpaperRotation::FlipHorizontal:
            result = img.flipped(Qt::Horizontal); break;
        case WallpaperRotation::FlipVertical:
            result = img.flipped(Qt::Vertical); break;
        default: return QString();
    }
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/HyprWall";
    QDir().mkpath(cacheDir);
    QString tmp = cacheDir + "/" + QFileInfo(src).baseName() + "_rot.png";
    if (!result.save(tmp, "PNG")) { qWarning() << "prepareRotatedImage: save failed" << tmp; return QString(); }
    return tmp;
}

// ── IPC helpers ────────────────────────────────────────────────────────────
static QString ipcRun(const QStringList &args)
{
    QProcess p;
    p.start("hyprctl", args);
    p.waitForFinished(4000);
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

static bool ipcOk(const QString &out)
{
    QString l = out.toLower();
    return !l.startsWith("error") && !l.contains("invalid");
}

static bool waitForHyprpaper(int maxMs = 6000)
{
    int waited = 0;
    while (waited < maxMs) {
        QThread::msleep(250);
        waited += 250;
        QString out = ipcRun({"hyprpaper", "listloaded"});
        if (!out.toLower().startsWith("error") && !out.toLower().contains("invalid")) {
            qDebug() << "[IPC] hyprpaper ready after" << waited << "ms, listloaded:" << out;
            return true;
        }
    }
    qWarning() << "[IPC] hyprpaper did not become ready in" << maxMs << "ms";
    return false;
}

static bool applyViaIPC(const QString &monitor, const QString &path)
{
    QString preOut = ipcRun({"hyprpaper", "preload", path});
    qDebug() << "[IPC] preload" << path << "->" << preOut;
    if (!preOut.isEmpty() && !ipcOk(preOut)) {
        qWarning() << "[IPC] preload failed:" << preOut;
        return false;
    }
    QString wpArg = monitor + "," + path;
    QString wpOut = ipcRun({"hyprpaper", "wallpaper", wpArg});
    qDebug() << "[IPC] wallpaper" << wpArg << "->" << wpOut;
    if (!wpOut.isEmpty() && !ipcOk(wpOut)) {
        qWarning() << "[IPC] wallpaper failed:" << wpOut;
        return false;
    }
    return true;
}

static void ipcUnloadOthers(const QString &keepPath)
{
    QString loaded = ipcRun({"hyprpaper", "listloaded"});
    if (loaded.isEmpty()) return;
    for (const QString &line : loaded.split('\n', Qt::SkipEmptyParts)) {
        QString img = line.trimmed();
        if (!img.isEmpty() && img != keepPath)
            ipcRun({"hyprpaper", "unload", img});
    }
}

// Write hyprpaper.conf for ALL configured monitors and return the conf path
static QString writeHyprpaperConf(const QMap<QString, WallpaperConfig> &all)
{
    QString confDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/hypr";
    QDir().mkpath(confDir);
    QString confPath = confDir + "/hyprpaper.conf";
    QFile f(confPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[conf] cannot write" << confPath;
        return {};
    }
    QTextStream ts(&f);
    ts << "# Generated by HyprWall\nsplash = false\nipc = on\n\n";
    for (auto it = all.cbegin(); it != all.cend(); ++it) {
        const WallpaperConfig &c = it.value();
        if (c.filePath.isEmpty() || WallpaperApplier::isVideoFile(c.filePath)) continue;
        QString path = c.filePath;
        QString rot  = WallpaperApplier::prepareRotatedImage(path, c.rotation);
        if (!rot.isEmpty()) path = rot;
        ts << "preload = " << path << "\n";
        ts << "wallpaper = " << c.monitorName << "," << path << "\n\n";
        qDebug() << "[conf] monitor" << c.monitorName << "->" << path;
    }
    f.close();
    qDebug() << "[conf] written to" << confPath;
    return confPath;
}

static bool restartHyprpaper()
{
    QProcess::execute("pkill", {"-x", "hyprpaper"});
    QThread::msleep(400);
    bool started = QProcess::startDetached("hyprpaper", {});
    if (!started) { qWarning() << "[apply] failed to start hyprpaper"; return false; }
    qDebug() << "[apply] hyprpaper started, waiting for IPC...";
    return waitForHyprpaper(6000);
}

// ── public apply ────────────────────────────────────────────────────────────
bool WallpaperApplier::apply(const WallpaperConfig &cfg)
{
    if (cfg.filePath.isEmpty()) { qWarning() << "apply: empty filePath"; return false; }

    if (isVideoFile(cfg.filePath)) {
        stopVideo(cfg.monitorName);
        QString opts = mpvOptions(
            static_cast<int>(cfg.fillMode),
            static_cast<int>(cfg.rotation),
            cfg.audioEnabled, cfg.audioVolume);
        QStringList args;
        args << "-o" << opts << cfg.monitorName << cfg.filePath;
        qDebug() << "mpvpaper" << args;
        return QProcess::startDetached("mpvpaper", args);
    }

    QString path = cfg.filePath;
    QString rotPath = prepareRotatedImage(path, cfg.rotation);
    if (!rotPath.isEmpty()) path = rotPath;

    // Ensure this monitor's config is in ConfigManager before writing conf
    auto &cm = ConfigManager::instance();
    {
        WallpaperConfig saved = cfg;
        saved.filePath = path;
        cm.setConfig(cfg.monitorName, saved);
    }

    // Check if hyprpaper is running and IPC works
    QProcess chk;
    chk.start("pgrep", {"-x", "hyprpaper"});
    chk.waitForFinished(1000);
    bool running = !chk.readAllStandardOutput().trimmed().isEmpty();

    if (running && applyViaIPC(cfg.monitorName, path)) {
        ipcUnloadOthers(path);
        return true;
    }

    // IPC failed or not running: write full conf for all monitors, then restart
    qDebug() << "[apply] (re)starting hyprpaper via conf for" << cm.configs().size() << "monitor(s)";
    writeHyprpaperConf(cm.configs());

    if (!restartHyprpaper()) return false;

    // After restart conf already applied wallpapers; send IPC too for current monitor
    bool ok = applyViaIPC(cfg.monitorName, path);
    ipcUnloadOthers(path);
    return ok;
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
    cfg.audioEnabled = !cfg.audioEnabled;
    cm.setConfig(monitor, cfg);
    cm.save();
    apply(cfg);
}
