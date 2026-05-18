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
        m.x           = obj["x"].toInt();
        m.y           = obj["y"].toInt();
        m.scale       = obj["scale"].toDouble(1.0);
        m.refreshRate = (int)obj["refreshRate"].toDouble(60.0);
        m.connected   = true;

        // hyprctl отдаёт физические размеры матрицы (до поворота).
        // При transform 1 или 3 (90°/270°) логическая ширина и высота меняются местами.
        int rawW = obj["width"].toInt();
        int rawH = obj["height"].toInt();
        if (m.transform % 2 == 1) {
            // повёрнут на 90 или 270 — swap
            m.width  = rawH;
            m.height = rawW;
        } else {
            m.width  = rawW;
            m.height = rawH;
        }
        result.append(m);
    }
    return result;
}
