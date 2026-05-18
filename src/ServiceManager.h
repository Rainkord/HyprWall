#pragma once
#include "Types.h"
#include <QList>

class ServiceManager {
public:
    static void applyAll(const QList<MonitorInfo> &monitors);
};
