#pragma once
#include "Types.h"
#include <QList>

class ServiceManager {
public:
    // Called by --daemon: restores all configured wallpapers after login.
    // If slideshow was enabled, picks a random gallery item immediately.
    static void applyAll(const QList<MonitorInfo> &monitors);
};
