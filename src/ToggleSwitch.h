#pragma once
#include <QWidget>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <QPainter>
#include <QRadialGradient>

class ToggleSwitch : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int knobX READ knobX WRITE setKnobX)
public:
    explicit ToggleSwitch(QWidget *parent = nullptr)
        : QWidget(parent), m_checked(false), m_knobX(3)
    {
        setFixedSize(44, 26);
        setCursor(Qt::PointingHandCursor);
        m_anim = new QPropertyAnimation(this, "knobX", this);
        m_anim->setDuration(160);
        m_anim->setEasingCurve(QEasingCurve::OutCubic);
    }
    bool isChecked() const { return m_checked; }
    void setChecked(bool on, bool animated = true) {
        if (m_checked == on) return;
        m_checked = on;
        int target = on ? (width() - 22) : 3;
        if (animated) {
            m_anim->stop();
            m_anim->setStartValue(m_knobX);
            m_anim->setEndValue(target);
            m_anim->start();
        } else {
            m_knobX = target;
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

        // Track
        QColor track = m_checked ? QColor(0x23,0x86,0x36,230) : QColor(0x30,0x36,0x3d,220);
        p.setBrush(track); p.setPen(Qt::NoPen);
        p.drawRoundedRect(0, 0, width(), height(), height()/2, height()/2);

        // Inner glow when checked
        if (m_checked) {
            QLinearGradient glow(0, 0, width(), 0);
            glow.setColorAt(0.0, QColor(0x3f,0xb9,0x50,40));
            glow.setColorAt(1.0, QColor(0x23,0x86,0x36,40));
            p.setBrush(glow);
            p.drawRoundedRect(1, 1, width()-2, height()-2, height()/2, height()/2);
        }

        // Knob shadow
        p.setBrush(QColor(0, 0, 0, 40));
        p.drawEllipse(m_knobX + 1, 4, 18, 18);

        // Knob
        p.setBrush(QColor(0xf0, 0xf6, 0xfc, 240));
        p.drawEllipse(m_knobX, 3, 18, 18);

        // Subtle inner highlight on knob
        QRadialGradient kg(m_knobX + 9, 10, 9);
        kg.setColorAt(0.0, QColor(255, 255, 255, 30));
        kg.setColorAt(1.0, QColor(255, 255, 255, 0));
        p.setBrush(kg);
        p.drawEllipse(m_knobX, 3, 18, 18);
    }
private:
    bool   m_checked;
    int    m_knobX;
    QPropertyAnimation *m_anim;
};
