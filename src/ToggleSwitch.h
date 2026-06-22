#pragma once
#include <QWidget>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QLinearGradient>

class ToggleSwitch : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int knobX READ knobX WRITE setKnobX)
    Q_PROPERTY(float trackGlow READ trackGlow WRITE setTrackGlow)
public:
    explicit ToggleSwitch(QWidget *parent = nullptr)
        : QWidget(parent), m_checked(false), m_knobX(3), m_trackGlow(0.f)
    {
        setFixedSize(46, 26);
        setCursor(Qt::PointingHandCursor);
        m_anim = new QPropertyAnimation(this, "knobX", this);
        m_anim->setDuration(200);
        m_anim->setEasingCurve(QEasingCurve::OutBack);
        m_glowAnim = new QPropertyAnimation(this, "trackGlow", this);
        m_glowAnim->setDuration(350);
        m_glowAnim->setEasingCurve(QEasingCurve::OutCubic);
    }
    bool isChecked() const { return m_checked; }
    float trackGlow() const { return m_trackGlow; }
    void setTrackGlow(float g) { m_trackGlow = g; update(); }
    void setChecked(bool on, bool animated = true) {
        if (m_checked == on) return;
        m_checked = on;
        int target = on ? (width() - 23) : 3;
        if (animated) {
            m_anim->stop();
            m_anim->setStartValue(m_knobX);
            m_anim->setEndValue(target);
            m_anim->start();
            m_glowAnim->stop();
            m_glowAnim->setStartValue(m_trackGlow);
            m_glowAnim->setEndValue(on ? 1.f : 0.f);
            m_glowAnim->start();
        } else {
            m_knobX = target;
            m_trackGlow = on ? 1.f : 0.f;
            update();
        }
        emit toggled(m_checked);
    }
    int  knobX() const { return m_knobX; }
    void setKnobX(int x) { m_knobX = x; update(); }
signals:
    void toggled(bool checked);
protected:
    void mousePressEvent(QMouseEvent *) override { setChecked(!m_checked); }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // Track background
        QColor trackBg = m_checked ? QColor(0x1a,0x3a,0x24,240) : QColor(0x21,0x26,0x2d,230);
        p.setBrush(trackBg);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(0, 0, width(), height(), height()/2, height()/2);

        // Animated track glow (expands from center when checked)
        if (m_trackGlow > 0.01f) {
            float glowAlpha = m_trackGlow * 60.f;
            QLinearGradient glow(0, 0, width(), 0);
            glow.setColorAt(0.0, QColor(0x2e,0xa0,0x43, (int)glowAlpha));
            glow.setColorAt(0.5, QColor(0x3f,0xb9,0x50, (int)(glowAlpha * 1.3f)));
            glow.setColorAt(1.0, QColor(0x23,0x86,0x36, (int)glowAlpha));
            p.setBrush(glow);
            p.drawRoundedRect(1, 1, width()-2, height()-2, height()/2, height()/2);
        }

        // Track border
        QColor border = m_checked ? QColor(0x3f,0xb9,0x50,120) : QColor(0x30,0x36,0x3d,180);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(border, 1));
        p.drawRoundedRect(0.5, 0.5, width()-1, height()-1, height()/2, height()/2);

        // Knob shadow
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 50));
        p.drawEllipse(m_knobX + 1, 4, 18, 18);

        // Knob body
        p.setBrush(QColor(0xf0, 0xf6, 0xfc, 245));
        p.drawEllipse(m_knobX, 3, 18, 18);

        // Knob radial highlight
        QRadialGradient kg(m_knobX + 9, 10, 10);
        kg.setColorAt(0.0, QColor(255, 255, 255, 45));
        kg.setColorAt(0.6, QColor(255, 255, 255, 10));
        kg.setColorAt(1.0, QColor(255, 255, 255, 0));
        p.setBrush(kg);
        p.drawEllipse(m_knobX, 3, 18, 18);

        // Knob ring when checked
        if (m_checked) {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(QColor(0x3f,0xb9,0x50, (int)(m_trackGlow * 80)), 1));
            p.drawEllipse(m_knobX + 1, 4, 16, 16);
        }
    }
private:
    bool   m_checked;
    int    m_knobX;
    float  m_trackGlow;
    QPropertyAnimation *m_anim;
    QPropertyAnimation *m_glowAnim;
};
