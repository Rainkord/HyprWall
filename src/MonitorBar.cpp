#include "MonitorBar.h"
#include <climits>
#include <algorithm>
#include <cmath>

QList<MonitorBar::MonitorRect> MonitorBar::computeMonitorRects() const {
    QList<MonitorRect> result;
    if (m_monitors.isEmpty()) return result;
    int mnX=INT_MAX,mnY=INT_MAX,mxX=INT_MIN,mxY=INT_MIN;
    for (auto &m:m_monitors){mnX=std::min(mnX,m.x);mnY=std::min(mnY,m.y);mxX=std::max(mxX,m.x+m.width);mxY=std::max(mxY,m.y+m.height);}
    int tW=mxX-mnX,tH=mxY-mnY; if(!tW||!tH) return result;
    const int P=16; int aW=width()-2*P,aH=height()-2*P;
    double sc=std::min((double)aW/tW,(double)aH/tH);
    int oX=P+(aW-(int)(tW*sc))/2,oY=P+(aH-(int)(tH*sc))/2;
    for (auto &m:m_monitors){
        int rx=oX+(int)((m.x-mnX)*sc),ry=oY+(int)((m.y-mnY)*sc);
        int rw=std::max(6,(int)(m.width*sc)),rh=std::max(6,(int)(m.height*sc));
        result.append({QRect(rx,ry,rw,rh), m.name});
    }
    return result;
}

void MonitorBar::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // Background — slightly lighter, rounded 12px
    p.setPen(QPen(QColor(0x21,0x26,0x2d,200),1));
    p.setBrush(QColor(13,17,23,220));
    p.drawRoundedRect(rect().adjusted(0,0,-1,-1),12,12);

    if (m_monitors.isEmpty()) {
        p.setPen(QColor(0x8b,0x94,0x9e));
        p.drawText(rect(),Qt::AlignCenter,m_noMon); return;
    }
    QList<MonitorRect> rects = computeMonitorRects();
    for (auto &mr : rects){
        QRect r = mr.rect; bool sel=(mr.name==m_selected);
        int mode=m_modes.value(mr.name,-1);
        int rw=r.width(), rh=r.height();

        // Rounded clip for monitor previews
        QPainterPath monPath;
        monPath.addRoundedRect(QRectF(r), 4, 4);
        p.setClipPath(monPath);

        if (mode==0 && m_pixmaps.contains(mr.name)) {
            const QPixmap &px=m_pixmaps[mr.name];
            QSize sc2=px.size().scaled(r.size(),Qt::KeepAspectRatioByExpanding);
            QPixmap sp=px.scaled(sc2,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
            int cx=(sp.width()-rw)/2,cy=(sp.height()-rh)/2;
            p.drawPixmap(r.topLeft(),sp,QRect(cx,cy,rw,rh));
        } else if (mode==1) {
            p.fillRect(r,QColor(16,10,30));
            QFont f=p.font(); f.setPointSize(std::max(8,rh/5)); p.setFont(f);
            p.setPen(QColor(139,92,246));
            p.drawText(r,Qt::AlignCenter,"\u25b6");
        } else if (mode==2) {
            p.fillRect(r, QColor(13,17,23));
            const int cols=2, rows=2;
            int cw=rw/cols, ch=rh/rows;
            p.setPen(QPen(QColor(0x30,0x36,0x3d,180),1));
            for(int ci=1;ci<cols;++ci)
                p.drawLine(r.left()+ci*cw, r.top(), r.left()+ci*cw, r.bottom());
            for(int ri=1;ri<rows;++ri)
                p.drawLine(r.left(), r.top()+ri*ch, r.right(), r.top()+ri*ch);
            p.setBrush(QColor(22,27,34));
            p.setPen(Qt::NoPen);
            for(int ci=0;ci<cols;++ci)
                for(int ri=0;ri<rows;++ri)
                    p.drawRect(r.left()+ci*cw+1, r.top()+ri*ch+1, cw-2, ch-2);
            int badgeS=std::min(rh/3, 18);
            QRect badge(r.right()-badgeS-2, r.bottom()-badgeS-2, badgeS, badgeS);
            p.setBrush(QColor(0x23,0x86,0x36,220));
            p.setPen(Qt::NoPen);
            p.drawEllipse(badge);
            p.setPen(QColor(0xf0,0xf6,0xfc,230));
            QFont bf=p.font(); bf.setPointSize(std::max(6,badgeS*5/9)); bf.setBold(true); p.setFont(bf);
            p.drawText(badge, Qt::AlignCenter, "\u21ba");
        } else {
            p.fillRect(r,QColor(22,27,34));
        }
        p.setClipping(false);

        // Rounded border — pulsing glow on selected
        QPainterPath borderPath;
        borderPath.addRoundedRect(QRectF(r), 4, 4);
        if(sel){
            float t = std::sin(m_glowPhase) * 0.5f + 0.5f;
            int alpha = (int)(140 + t * 80);
            p.setPen(QPen(QColor(0x58,0xa6,0xff, alpha), 2));
            p.setBrush(Qt::NoBrush);
            p.drawPath(borderPath);
            // Outer glow
            QRadialGradient glow(r.center(), std::max(r.width(), r.height()) * 0.7);
            glow.setColorAt(0.0, QColor(0x58,0xa6,0xff, (int)(30 * t)));
            glow.setColorAt(1.0, QColor(0x58,0xa6,0xff, 0));
            p.setPen(Qt::NoPen);
            p.setBrush(glow);
            p.drawRoundedRect(r.adjusted(-4,-4,4,4), 6, 6);
        }
        else   {p.setPen(QPen(QColor(0x30,0x36,0x3d,180),1));p.setBrush(Qt::NoBrush);p.drawPath(borderPath);}

        // Name label — better styling
        int lH=std::min(20,rh); QRect lr(r.left(),r.top()+rh-lH,rw,lH);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0,0,0,180));
        p.drawRect(lr);
        p.setPen(sel?QColor(0x58,0xa6,0xff):QColor(0xc9,0xd1,0xd9));
        QFont nf=p.font(); nf.setPointSize(8); nf.setBold(sel); p.setFont(nf);
        p.drawText(lr,Qt::AlignCenter,mr.name);
    }
}

void MonitorBar::mousePressEvent(QMouseEvent *ev) {
    if(m_monitors.isEmpty()) return;
    QList<MonitorRect> rects = computeMonitorRects();
    for (auto &mr : rects){
        if(mr.rect.contains(ev->pos())){emit monitorClicked(mr.name);return;}
    }
}
