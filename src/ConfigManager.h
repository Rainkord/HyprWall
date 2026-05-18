#pragma once
#include "Types.h"
#include <QMap>
#include <QString>

class ConfigManager {
public:
    static ConfigManager& instance();

    void load();
    void save();

    WallpaperConfig getConfig(const QString &monitor) const;
    void            setConfig(const QString &monitor, const WallpaperConfig &cfg);

    QMap<QString, WallpaperConfig>& configs() { return m_configs; }

    static QString configPath();

private:
    ConfigManager() = default;
    QMap<QString, WallpaperConfig> m_configs;
};
