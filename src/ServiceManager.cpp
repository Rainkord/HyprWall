#include "ServiceManager.h"
#include "ConfigManager.h"
#include "WallpaperApplier.h"

void ServiceManager::applyAll(const QList<MonitorInfo> &monitors)
{
    ConfigManager &cm = ConfigManager::instance();
    cm.load();
    for (const MonitorInfo &m : monitors) {
        WallpaperConfig cfg = cm.getConfig(m.name);
        if (!cfg.filePath.isEmpty())
            WallpaperApplier::apply(cfg);
    }
}
