#include "ConfigManager.h"
#include "WallpaperApplier.h"
#include "ImageCache.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QSaveFile>
#include <QMutex>
#include <QDateTime>
#include <QDebug>
#include <QRegularExpression>

ConfigManager& ConfigManager::instance()
{
    static ConfigManager inst;
    return inst;
}

QString ConfigManager::configPath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/hyprwall";
    QDir().mkpath(dir);
    return dir + "/config.ini";
}

QString ConfigManager::galleryDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                  + "/hyprwall/gallery";
    QDir().mkpath(dir);
    return dir;
}

WallpaperConfig ConfigManager::getConfig(const QString &monitor) const
{
    if (m_configs.contains(monitor))
        return m_configs.value(monitor);
    WallpaperConfig cfg;
    cfg.monitorName = monitor;
    return cfg;
}

void ConfigManager::setConfig(const QString &monitor, const WallpaperConfig &cfg)
{
    m_configs[monitor] = cfg;
}

WallpaperConfig ConfigManager::getHyprlockConfig(const QString &monitor) const
{
    if (m_hyprlockConfigs.contains(monitor))
        return m_hyprlockConfigs.value(monitor);
    WallpaperConfig cfg;
    cfg.monitorName = monitor;
    return cfg;
}

void ConfigManager::setHyprlockConfig(const QString &monitor, const WallpaperConfig &cfg)
{
    m_hyprlockConfigs[monitor] = cfg;
}

void ConfigManager::load()
{
    const QString path = configPath();
    if (!loadFromFile(path)) {
        // Main config corrupt — try backup
        const QString bakPath = path + ".bak";
        if (QFile::exists(bakPath)) {
            qWarning() << "ConfigManager: main config corrupt, restoring from .bak";
            QFile::remove(path);
            QFile::rename(bakPath, path);
            loadFromFile(path);
        }
    }
}

bool ConfigManager::loadFromFile(const QString &path)
{
    QSettings s(path, QSettings::IniFormat);
    if (s.status() != QSettings::NoError) return false;

    for (const QString &mon : s.childGroups()) {
        if (mon == "General") {
            s.beginGroup("General");
            for (const QString &key : s.childKeys())
                m_general[key] = s.value(key);
            s.endGroup();
            continue;
        }
        s.beginGroup(mon);
        WallpaperConfig cfg;
        cfg.monitorName         = mon;
        cfg.filePath            = s.value("filePath").toString();
        cfg.fillMode            = static_cast<FillMode>(s.value("fillMode", 0).toInt());
        cfg.rotation            = static_cast<WallpaperRotation>(s.value("rotation", 0).toInt());
        cfg.audioEnabled        = s.value("audioEnabled",       false).toBool();
        cfg.audioVolume         = s.value("audioVolume",        50).toInt();
        cfg.slideshowEnabled    = s.value("slideshowEnabled",   false).toBool();
        cfg.slideshowInterval   = s.value("slideshowInterval",  300).toInt();
        cfg.slideshowMode       = s.value("slideshowMode",      2).toInt();
        s.endGroup();

        if (mon.startsWith("Hyprlock-")) {
            QString monitorName = mon.mid(9); // strip "Hyprlock-"
            cfg.monitorName = monitorName;
            m_hyprlockConfigs[monitorName] = cfg;
        } else {
            m_configs[mon] = cfg;
        }
    }
    return true;
}

void ConfigManager::save()
{
    const QString path = configPath();
    const QString bakPath = path + ".bak";

    // Rotate: current → .bak (for crash recovery rollback)
    if (QFile::exists(path)) {
        QFile::remove(bakPath);
        QFile::rename(path, bakPath);
    }

    // Atomic write: QSaveFile guarantees no partial/corrupt config
    QSaveFile sf(path);
    if (!sf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "ConfigManager::save: cannot open" << path << sf.errorString();
        return;
    }
    QTextStream ts(&sf);
    // General settings
    if (!m_general.isEmpty()) {
        ts << "[General]\n";
        for (auto it = m_general.cbegin(); it != m_general.cend(); ++it)
            ts << it.key() << "=" << it.value().toString() << "\n";
    }
    for (auto it = m_configs.cbegin(); it != m_configs.cend(); ++it) {
        const WallpaperConfig &cfg = it.value();
        ts << "[" << it.key() << "]\n";
        ts << "filePath=" << cfg.filePath << "\n";
        ts << "fillMode=" << static_cast<int>(cfg.fillMode) << "\n";
        ts << "rotation=" << static_cast<int>(cfg.rotation) << "\n";
        ts << "audioEnabled=" << (cfg.audioEnabled ? "true" : "false") << "\n";
        ts << "audioVolume=" << cfg.audioVolume << "\n";
        ts << "slideshowEnabled=" << (cfg.slideshowEnabled ? "true" : "false") << "\n";
        ts << "slideshowInterval=" << cfg.slideshowInterval << "\n";
        ts << "slideshowMode=" << cfg.slideshowMode << "\n";
    }
    // Hyprlock configs
    for (auto it = m_hyprlockConfigs.cbegin(); it != m_hyprlockConfigs.cend(); ++it) {
        const WallpaperConfig &cfg = it.value();
        ts << "[Hyprlock-" << it.key() << "]\n";
        ts << "filePath=" << cfg.filePath << "\n";
    }
    if (!sf.commit()) {
        qWarning() << "ConfigManager::save: commit failed" << sf.errorString();
        // Roll back from .bak if we renamed it
        if (QFile::exists(bakPath)) {
            QFile::remove(path);
            QFile::rename(bakPath, path);
        }
    }
}

QList<GalleryItem> ConfigManager::loadGallery() const
{
    QMutexLocker lock(&m_galleryMutex);

    // Check cache validity: only rescan if directory mtime changed
    QDir d(galleryDir());
    QDateTime currentMtime;
    const auto entries = d.entryInfoList(QDir::Files);
    if (!entries.isEmpty())
        currentMtime = entries.last().lastModified();  // most recent file change
    if (!m_galleryCache.isEmpty() && currentMtime == m_galleryDirMtime)
        return m_galleryCache;

    const QStringList imageExts = {"jpg","jpeg","png","bmp","webp","tiff"};
    const QStringList videoExts = {"mp4","mkv","avi","webm","mov","flv","wmv"};
    d.setSorting(QDir::Time | QDir::Reversed);
    QStringList allExts;
    for (auto &e : imageExts) allExts << ("*." + e) << ("*." + e.toUpper());
    for (auto &e : videoExts) allExts << ("*." + e) << ("*." + e.toUpper());
    d.setNameFilters(allExts);

    m_galleryCache.clear();
    for (const QFileInfo &fi : entries) {
        GalleryItem item;
        item.path    = fi.absoluteFilePath();
        item.isVideo = videoExts.contains(fi.suffix().toLower());
        m_galleryCache << item;
    }
    m_galleryDirMtime = currentMtime;
    return m_galleryCache;
}

QList<GalleryItem> ConfigManager::addToGallery(const QStringList &paths)
{
    static constexpr qint64 MAX_FILE_SIZE = 500LL * 1024 * 1024; // 500 MB
    QString dest = galleryDir();
    QList<GalleryItem> added;
    for (const QString &src : paths) {
        QFileInfo fi(src);
        if (!fi.exists() || !fi.isFile()) continue;

        // Resolve symlinks to prevent symlink attack
        QString realPath = fi.isSymLink() ? fi.symLinkTarget() : fi.absoluteFilePath();
        QFileInfo realFi(realPath);
        if (!realFi.exists() || !realFi.isFile()) continue;

        // Size check
        if (realFi.size() > MAX_FILE_SIZE) {
            qWarning() << "addToGallery: file too large, skipped:" << src
                        << "(" << realFi.size() / (1024*1024) << "MB)";
            continue;
        }

        // Validate extension before copy
        QStringList validExts = {"jpg","jpeg","png","bmp","webp","gif",
                                 "mp4","mkv","avi","webm","mov"};
        if (!validExts.contains(realFi.suffix().toLower())) continue;

        QString dstPath = dest + "/" + realFi.fileName();
        int n = 1;
        while (QFile::exists(dstPath))
            dstPath = dest + "/" + realFi.completeBaseName() + QString("_%1.").arg(n++) + realFi.suffix();
        if (QFile::copy(realPath, dstPath)) {
            GalleryItem item;
            item.path    = dstPath;
            item.isVideo = WallpaperApplier::isVideoFile(dstPath);
            added << item;
        } else {
            qWarning() << "addToGallery: copy failed" << src << "->" << dstPath;
        }
    }
    invalidateGalleryCache();
    return added;
}

void ConfigManager::removeFromGallery(const QString &path)
{
    QFileInfo fi(path);
    QString gallery = QFileInfo(galleryDir()).absoluteFilePath();
    if (!fi.absoluteFilePath().startsWith(gallery)) {
        qWarning() << "removeFromGallery: refused, path outside gallery:" << path;
        return;
    }
    QFile::remove(path);
    invalidateGalleryCache();
}

void ConfigManager::invalidateGalleryCache() const
{
    QMutexLocker lock(&m_galleryMutex);
    m_galleryCache.clear();
    m_galleryDirMtime = QDateTime();
}

void ConfigManager::writeHyprlockConf()
{
    QString hyprlockPath = QDir::homePath() + "/.config/hypr/hyprlock.conf";
    QFile readFile(hyprlockPath);
    if (!readFile.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QString content = QString::fromUtf8(readFile.readAll());
    readFile.close();

    // Remove all existing background { } blocks (greedy match between first and last)
    static QRegularExpression reBg(R"(background\s*\{[^}]*\})");
    content.remove(reBg);

    // Build new background blocks from hyprlock configs
    // Preserve the order from the existing file's comment markers, but fallback to
    // monitor order: DP-1, DP-2, DP-3, HDMI-A-1
    QStringList monitorOrder = {"DP-1", "DP-2", "DP-3", "HDMI-A-1"};
    // Also add any monitors in hyprlockConfigs that aren't in the standard order
    for (auto it = m_hyprlockConfigs.constBegin(); it != m_hyprlockConfigs.constEnd(); ++it) {
        if (!monitorOrder.contains(it.key()))
            monitorOrder.append(it.key());
    }

    QString newBlocks;
    for (const QString &mon : monitorOrder) {
        if (!m_hyprlockConfigs.contains(mon)) continue;
        const WallpaperConfig &cfg = m_hyprlockConfigs[mon];
        if (cfg.filePath.isEmpty()) continue;

        QString imgPath = ImageCache::getCompressedOrOriginal(cfg.filePath);

        newBlocks += QString(
            "background {\n"
            "    monitor = %1\n"
            "    path = %2\n"
            "    blur_passes = 3\n"
            "    contrast = 0.8916\n"
            "    brightness = 0.8172\n"
            "    vibrancy = 0.1696\n"
            "    vibrancy_darkness = 0.0\n"
            "}\n\n"
        ).arg(mon, imgPath);
    }

    // Insert new background blocks after the first comment section header or at top
    int insertPos = 0;
    // Find the position right after "# =========================\n# ФОНЫ\n# ========================="
    static QRegularExpression reHeader(R"(# =+\n# ФОНЫ\n# =+\n)");
    QRegularExpressionMatch match = reHeader.match(content);
    if (match.hasMatch()) {
        insertPos = match.capturedEnd();
    }

    content.insert(insertPos, newBlocks);

    // Clean up excessive blank lines (more than 2 consecutive)
    static QRegularExpression reBlank("\n{4,}");
    content.replace(reBlank, "\n\n\n");

    QSaveFile writeFile(hyprlockPath);
    if (!writeFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    writeFile.write(content.toUtf8());
    writeFile.commit();
}

QVariant ConfigManager::getSetting(const QString &key, const QVariant &def) const
{
    return m_general.value(key, def);
}

void ConfigManager::setSetting(const QString &key, const QVariant &val)
{
    m_general[key] = val;
}

int ConfigManager::loadLanguage()
{
    QString path = configPath().replace("config.ini", "language");
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return 0;
    bool ok = false;
    int lang = QString::fromUtf8(f.readAll()).trimmed().toInt(&ok);
    return ok ? lang : 0;
}

void ConfigManager::saveLanguage(int langIndex)
{
    QString path = configPath().replace("config.ini", "language");
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        f.write(QByteArray::number(langIndex));
}

bool ConfigManager::loadSameWallpaper()
{
    QString path = configPath().replace("config.ini", "samewallpaper");
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QString val = QString::fromUtf8(f.readAll()).trimmed();
    return val == "1" || val.toLower() == "true";
}

void ConfigManager::saveSameWallpaper(bool on)
{
    QString path = configPath().replace("config.ini", "samewallpaper");
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        f.write(on ? "1" : "0");
}
