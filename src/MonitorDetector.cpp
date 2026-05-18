#include "MonitorDetector.h"
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

QList<MonitorInfo> MonitorDetector::detect()
{
    QList<MonitorInfo> result;

    QProcess proc;
    proc.start("hyprctl", {"monitors", "-j"});
    if (!proc.waitForFinished(3000)) {
        qWarning() << "hyprctl timeout";
        return result;
    }

    QByteArray out = proc.readAllStandardOutput();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(out, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << err.errorString();
        return result;
    }

    for (const QJsonValue &v : doc.array()) {
        QJsonObject obj = v.toObject();
        MonitorInfo m;
        m.name        = obj["name"].toString();
        m.width       = obj["width"].toInt();
        m.height      = obj["height"].toInt();
        m.x           = obj["x"].toInt();
        m.y           = obj["y"].toInt();
        m.scale       = obj["scale"].toDouble(1.0);
        m.refreshRate = obj["refreshRate"].toInt(60);
        m.connected   = true;
        result.append(m);
    }
    return result;
}
