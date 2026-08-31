#pragma once
#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFont>
#include <QMap>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include "Types.h"
#include "ThumbCache.h"

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
    ~MonitorBar() {
        for (auto *w : m_loaders.values())
            w->deleteLater();
    }
    float glowPhase() const { return m_glowPhase; }
    void setGlowPhase(float v) { m_glowPhase = v; update(); }
    void setMonitors(const QList<MonitorInfo> &m)
        { m_monitors=m; m_selected=m.isEmpty()?QString():m.first().name; update(); }
    void setSelected(const QString &n) { m_selected=n; update(); }
    void setNoMonitorsText(const QString &t) { m_noMon=t; update(); }

    // mode: -1=blank, 0=static image, 1=video, 2=slideshow
    void setMonitorMode(const QString &mon, int mode, const QString &imgPath={},
                        int fillMode=0, int rotation=0)
    {
        m_modes[mon]=mode;
        m_fillModes[mon]=fillMode;
        if (mode==0 && !imgPath.isEmpty()) {
            QString cacheKey = QString("%1|%2|%3").arg(imgPath).arg(fillMode).arg(rotation);
            // Check in-memory cache
            if (m_cache.contains(cacheKey)) {
                m_pixmaps[mon] = m_cache[cacheKey];
                update();
                return;
            }
            // Check disk cache
            QPixmap diskCached = ThumbCache::load(imgPath, 400, 225, fillMode, rotation);
            if (!diskCached.isNull()) {
                m_cache[cacheKey] = diskCached;
                m_pixmaps[mon] = diskCached;
                update();
                return;
            }
            // Async load + save to disk cache
            if (m_loaders.contains(mon)) {
                m_loaders[mon]->deleteLater();
                m_loaders.remove(mon);
            }
            auto *watcher = new QFutureWatcher<QPixmap>(this);
            m_loaders[mon] = watcher;
            connect(watcher, &QFutureWatcher<QPixmap>::finished, this, [this, mon, watcher, cacheKey, imgPath, fillMode, rotation]() {
                if (m_loaders.value(mon) != watcher) { watcher->deleteLater(); return; }
                QPixmap thumb = watcher->result();
                if (!thumb.isNull()) {
                    m_pixmaps[mon] = thumb;
                    m_cache[cacheKey] = thumb;
                    // Save to disk cache
                    ThumbCache::save(imgPath, 400, 225, fillMode, rotation, thumb);
                }
                watcher->deleteLater();
                m_loaders.remove(mon);
                update();
            });
            watcher->setFuture(QtConcurrent::run([imgPath, fillMode, rotation]() -> QPixmap {
                // Use QImageReader::setScaledSize to handle huge PNGs (12000x14000)
                QPixmap thumb = ThumbCache::loadScaled(imgPath, 400, 225, rotation);
                return thumb;
            }));
        } else if (mode!=0) {
            if (m_loaders.contains(mon)) {
                m_loaders[mon]->deleteLater();
                m_loaders.remove(mon);
            }
            m_pixmaps.remove(mon);
            update();
        }
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
    QMap<QString,int>     m_fillModes;  // per-monitor fill mode for paint
    QMap<QString,QPixmap> m_pixmaps;
    QMap<QString,QPixmap> m_cache;  // key: "path|fillMode|rotation"
    QMap<QString, QFutureWatcher<QPixmap>*> m_loaders;
    float m_glowPhase = 0.f;
    QPropertyAnimation *m_glowAnim = nullptr;
};
