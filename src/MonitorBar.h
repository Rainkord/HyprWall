#pragma once
#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFont>
#include <QMap>
#include <QPixmap>
#include <QPropertyAnimation>
#include "Types.h"

class MonitorBar : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float glowPhase READ glowPhase WRITE setGlowPhase)
public:
    explicit MonitorBar(QWidget *p=nullptr) : QWidget(p) {
        setMinimumHeight(150);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setCursor(Qt::PointingHandCursor);
        m_glowAnim = new QPropertyAnimation(this, "glowPhase", this);
        m_glowAnim->setDuration(1500);
        m_glowAnim->setStartValue(0.f);
        m_glowAnim->setEndValue(6.2832f);
        m_glowAnim->setLoopCount(-1);
        m_glowAnim->setEasingCurve(QEasingCurve::Linear);
        m_glowAnim->start();
    }
    float glowPhase() const { return m_glowPhase; }
    void setGlowPhase(float v) { m_glowPhase = v; update(); }
    void setMonitors(const QList<MonitorInfo> &m)
        { m_monitors=m; m_selected=m.isEmpty()?QString():m.first().name; update(); }
    void setSelected(const QString &n) { m_selected=n; update(); }
    void setNoMonitorsText(const QString &t) { m_noMon=t; update(); }

    // mode: -1=blank, 0=static image, 1=video, 2=slideshow
    void setMonitorMode(const QString &mon, int mode, const QString &imgPath={})
    {
        m_modes[mon]=mode;
        if (mode==0 && !imgPath.isEmpty()) {
            QPixmap px(imgPath);
            if (!px.isNull()) m_pixmaps[mon]=px;
        } else if (mode!=0) {
            m_pixmaps.remove(mon);
        }
        update();
    }
signals:
    void monitorClicked(const QString &name);
protected:
    struct MonitorRect {
        QRect rect;
        QString name;
    };
    QList<MonitorRect> computeMonitorRects() const;
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent *ev) override;
private:
    QList<MonitorInfo> m_monitors;
    QString m_selected, m_noMon{"No monitors"};
    QMap<QString,int>     m_modes;
    QMap<QString,QPixmap> m_pixmaps;
    float m_glowPhase = 0.f;
    QPropertyAnimation *m_glowAnim = nullptr;
};
