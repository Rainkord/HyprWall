#pragma once
#include <QMap>
#include <QString>
#include "Types.h"

class ConfigManager {
public:
    static ConfigManager& instance();
    void load();
    void save();
    WallpaperConfig getConfig(const QString &monitor) const;
    void setConfig(const QString &monitor, const WallpaperConfig &cfg);
private:
    ConfigManager() = default;
    static QString configPath();
    QMap<QString, WallpaperConfig> m_configs;
};
