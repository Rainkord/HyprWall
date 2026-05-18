#include "ConfigManager.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

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

void ConfigManager::load()
{
    QSettings s(configPath(), QSettings::IniFormat);
    for (const QString &mon : s.childGroups()) {
        s.beginGroup(mon);
        WallpaperConfig cfg;
        cfg.monitorName   = mon;
        cfg.filePath      = s.value("filePath").toString();
        cfg.fillMode      = static_cast<FillMode>(s.value("fillMode", 0).toInt());
        cfg.rotation      = static_cast<WallpaperRotation>(s.value("rotation", 0).toInt());
        cfg.audioEnabled  = s.value("audioEnabled", false).toBool();
        cfg.audioVolume   = s.value("audioVolume", 50).toInt();
        m_configs[mon]    = cfg;
        s.endGroup();
    }
}

void ConfigManager::save()
{
    QSettings s(configPath(), QSettings::IniFormat);
    s.clear();
    for (auto it = m_configs.cbegin(); it != m_configs.cend(); ++it) {
        const WallpaperConfig &cfg = it.value();
        s.beginGroup(it.key());
        s.setValue("filePath",     cfg.filePath);
        s.setValue("fillMode",     static_cast<int>(cfg.fillMode));
        s.setValue("rotation",     static_cast<int>(cfg.rotation));
        s.setValue("audioEnabled", cfg.audioEnabled);
        s.setValue("audioVolume",  cfg.audioVolume);
        s.endGroup();
    }
}

WallpaperConfig ConfigManager::getConfig(const QString &monitor) const
{
    return m_configs.value(monitor, WallpaperConfig{monitor, "", FillMode::Fill});
}

void ConfigManager::setConfig(const QString &monitor, const WallpaperConfig &cfg)
{
    m_configs[monitor] = cfg;
}
