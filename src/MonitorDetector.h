#pragma once
#include "Types.h"
#include <QList>

class MonitorDetector {
public:
    static QList<MonitorInfo> detect();
};
