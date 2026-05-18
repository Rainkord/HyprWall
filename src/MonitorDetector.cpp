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
        m.description = obj["description"].toString();
        m.transform   = obj["transform"].toInt(0);

        // hyprctl даёт width/height уже с учётом transform
        // но позиции (x,y) — тоже логические
        m.width       = obj["width"].toInt();
        m.height      = obj["height"].toInt();
        m.x           = obj["x"].toInt();
        m.y           = obj["y"].toInt();
        m.scale       = obj["scale"].toDouble(1.0);
        // refreshRate может быть float (165.0) — берём как double и округляем
        m.refreshRate = (int)obj["refreshRate"].toDouble(60.0);
        m.connected   = true;
        result.append(m);
    }
    return result;
}
