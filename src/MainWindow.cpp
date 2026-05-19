#include "MainWindow.h"
#include "MonitorDetector.h"
#include "WallpaperApplier.h"
#include "ConfigManager.h"
#include "ServiceManager.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QPixmap>
#include <QTimer>
#include <QFrame>
#include <QPainter>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QMap>
#include <QTextStream>
#include <QFont>
#include <QSizePolicy>
#include <QPropertyAnimation>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <algorithm>
#include <climits>

const int MainWindow::INTERVAL_VALUES[] = { 60, 300, 600, 900, 1800, 3600 };

// Fixed thumbnail size — never changes regardless of window width
static constexpr int THUMB_W = 120;
static constexpr int THUMB_H = 68;   // ~16:9
static constexpr int GRID_W  = THUMB_W + 6;  // cell = thumb + 6px padding
static constexpr int GRID_H  = THUMB_H + 20; // cell height includes text label row

// ============================================================
// ToggleSwitch
// ============================================================
class ToggleSwitch : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int knobX READ knobX WRITE setKnobX)
public:
    explicit ToggleSwitch(QWidget *parent = nullptr)
        : QWidget(parent), m_checked(false), m_knobX(3)
    {
        setFixedSize(42, 24);
        setCursor(Qt::PointingHandCursor);
        m_anim = new QPropertyAnimation(this, "knobX", this);
        m_anim->setDuration(160);
        m_anim->setEasingCurve(QEasingCurve::OutCubic);
    }
    bool isChecked() const { return m_checked; }
    void setChecked(bool on, bool animated = true) {
        if (m_checked == on) return;
        m_checked = on;
        int target = on ? (width() - 21) : 3;
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
        QColor track = m_checked ? QColor(0x23,0x86,0x36,230) : QColor(0x30,0x36,0x3d,220);
        p.setBrush(track); p.setPen(Qt::NoPen);
        p.drawRoundedRect(0, 0, width(), height(), height()/2, height()/2);
        p.setBrush(QColor(0xf0,0xf6,0xfc,230));
        p.drawEllipse(m_knobX, 3, 18, 18);
    }
private:
    bool   m_checked;
    int    m_knobX;
    QPropertyAnimation *m_anim;
};

// ============================================================
// GalleryItemDelegate — draws thumbnail + × button
// ============================================================
#include <QStyledItemDelegate>
#include <QPainterPath>
class GalleryDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit GalleryDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *p, const QStyleOptionViewItem &opt,
               const QModelIndex &idx) const override
    {
        p->save();
        QRect r = opt.rect;

        // Background / selection
        bool sel = opt.state & QStyle::State_Selected;
        QColor bg = sel ? QColor(0x38,0x8b,0xfd,50) : QColor(22,27,34,180);
        p->setBrush(bg);
        p->setPen(QPen(sel ? QColor(0x58,0xa6,0xff,180) : QColor(0x30,0x36,0x3d,150), 1));
        QPainterPath path;
        path.addRoundedRect(QRectF(r).adjusted(0.5,0.5,-0.5,-0.5), 5, 5);
        p->setRenderHint(QPainter::Antialiasing);
        p->drawPath(path);

        // Thumbnail image (fills the whole cell except bottom label)
        QRect imgR = r.adjusted(1, 1, -1, -(GRID_H - THUMB_H - 1));
        QPixmap px = idx.data(Qt::DecorationRole).value<QIcon>().pixmap(THUMB_W, THUMB_H);
        if (!px.isNull()) {
            // Scale & crop to fill imgR exactly
            QPixmap scaled = px.scaled(imgR.size(),
                Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            int cx = (scaled.width()  - imgR.width())  / 2;
            int cy = (scaled.height() - imgR.height()) / 2;
            p->setClipRect(imgR);
            p->drawPixmap(imgR.topLeft(), scaled, QRect(cx, cy, imgR.width(), imgR.height()));
            p->setClipping(false);
        } else {
            // Placeholder while thumbnail loads
            p->fillRect(imgR, QColor(30, 35, 42));
            p->setPen(QColor(0x48,0x4f,0x58));
            p->drawText(imgR, Qt::AlignCenter, "…");
        }

        // Filename label
        QRect lblR(r.left(), r.bottom() - (GRID_H - THUMB_H - 2), r.width(), GRID_H - THUMB_H - 1);
        p->setPen(QColor(0x8b,0x94,0x9e));
        QFont f = p->font(); f.setPointSize(8); p->setFont(f);
        QString name = idx.data(Qt::DisplayRole).toString();
        p->drawText(lblR, Qt::AlignCenter | Qt::TextSingleLine,
            p->fontMetrics().elidedText(name, Qt::ElideRight, lblR.width() - 4));

        // × delete button (top-right corner)
        QRect xR(r.right() - 19, r.top() + 2, 17, 17);
        p->setBrush(QColor(218,54,51,160));
        p->setPen(Qt::NoPen);
        p->drawEllipse(xR);
        p->setPen(QColor(0xff,0xff,0xff,220));
        QFont xf = p->font(); xf.setPointSize(8); xf.setBold(true); p->setFont(xf);
        p->drawText(xR, Qt::AlignCenter, "\u00d7");

        // Dim overlay when slideshow locked
        bool locked = idx.data(Qt::UserRole + 2).toBool();
        if (locked) {
            p->setBrush(QColor(13,17,23,110));
            p->setPen(Qt::NoPen);
            p->drawPath(path);
        }

        p->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override {
        return QSize(GRID_W, GRID_H);
    }
};

// ============================================================
// Stylesheet
// ============================================================
static const char *APP_STYLE = R"(
* {
    font-family: 'Segoe UI', 'Inter', sans-serif;
    font-size: 13px;
    color: #c9d1d9;
}
QMainWindow, QWidget#central, QScrollArea, QWidget#scrollContents {
    background: transparent;
    border: none;
}
QGroupBox {
    background: rgba(22, 27, 34, 190);
    border: 1px solid #30363d;
    border-radius: 10px;
    margin-top: 18px;
    padding: 10px 8px 8px 8px;
    font-weight: 600;
    color: #8b949e;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 12px; top: 4px;
    color: #8b949e;
    font-size: 11px;
    letter-spacing: 1px;
    text-transform: uppercase;
}
QLineEdit {
    background: rgba(13,17,23,200);
    border: 1px solid #30363d; border-radius: 6px;
    padding: 0 10px; color: #c9d1d9;
    selection-background-color: #388bfd; selection-color: #fff;
    min-height: 32px; max-height: 32px;
}
QLineEdit:focus { border: 1px solid #58a6ff; background: rgba(13,17,23,230); }
QComboBox {
    background: rgba(22,27,34,220);
    border: 1px solid #30363d; border-radius: 6px;
    padding: 0 10px; color: #c9d1d9;
    min-height: 32px; max-height: 32px;
}
QComboBox:hover { border: 1px solid #58a6ff; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox::down-arrow {
    image: none;
    border-left: 4px solid transparent; border-right: 4px solid transparent;
    border-top: 5px solid #8b949e;
    width: 0; height: 0; margin-right: 8px;
}
QComboBox QAbstractItemView {
    background: #161b22; border: 1px solid #30363d; border-radius: 6px;
    selection-background-color: rgba(88,166,255,40);
    color: #c9d1d9; outline: none; padding: 2px;
}
QPushButton {
    background: rgba(48,54,61,220); border: 1px solid #30363d;
    border-radius: 6px; padding: 0 16px; color: #c9d1d9;
    font-weight: 500; min-height: 32px; max-height: 32px;
}
QPushButton:hover { background: rgba(56,139,253,30); border: 1px solid #388bfd; color: #58a6ff; }
QPushButton:pressed { background: rgba(56,139,253,20); }
QPushButton#galleryAddBtn {
    background: rgba(33,38,45,200);
    border: 1px solid #30363d;
    border-radius: 5px;
    color: #8b949e;
    font-weight: 400;
    font-size: 12px;
    padding: 0 10px;
    min-height: 24px; max-height: 24px;
    min-width: 0;
}
QPushButton#galleryAddBtn:hover {
    background: rgba(48,54,61,220);
    border-color: #484f58;
    color: #c9d1d9;
}
QPushButton#galleryAddBtn:pressed {
    background: rgba(40,46,54,230);
}
QCheckBox { spacing: 8px; color: #8b949e; min-height: 28px; }
QCheckBox::indicator {
    width: 15px; height: 15px;
    border: 1px solid #30363d; border-radius: 3px;
    background: rgba(13,17,23,200);
}
QCheckBox::indicator:checked { background: #238636; border-color: #2ea043; }
QSlider::groove:horizontal { height: 4px; background: #21262d; border-radius: 2px; }
QSlider::handle:horizontal {
    background: #58a6ff; border: 2px solid #0d1117;
    width: 13px; height: 13px; margin: -5px 0; border-radius: 7px;
}
QSlider::sub-page:horizontal { background: #388bfd; border-radius: 2px; }
QLabel#orientLabel { color: #8b949e; font-size: 11px; }
QLabel#galleryEmpty { color: #484f58; font-size: 12px; }
QListWidget#galleryList {
    background: transparent;
    border: none;
    outline: none;
}
QListWidget#galleryList::item {
    background: transparent;
    border: none;
    padding: 0;
}
QListWidget#galleryList::item:selected {
    background: transparent;
    border: none;
}
)";

static QString orientStr(int t, const Strings &s)
{
    switch(t){
        case 1: return s.orientPortrait90;
        case 2: return s.orientLandscape180;
        case 3: return s.orientPortrait270;
        default: return s.orientLandscape;
    }
}

static QString autostartPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + "/autostart/hyprwall.desktop";
}
static bool autostartEnabled()
{
    return QFileInfo::exists(autostartPath());
}
static void setAutostart(bool en)
{
    QString path = autostartPath();
    if (en) {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&f);
            ts << "[Desktop Entry]\n"
               << "Type=Application\n"
               << "Name=HyprWall\n"
               << "Exec=hyprwall --daemon\n"
               << "X-GNOME-Autostart-enabled=true\n";
        }
    } else {
        QFile::remove(path);
    }
}

// ============================================================
// MonitorBar
// ============================================================
class MonitorBar : public QWidget {
    Q_OBJECT
public:
    explicit MonitorBar(QWidget *p=nullptr) : QWidget(p) {
        setMinimumHeight(150);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setCursor(Qt::PointingHandCursor);
    }
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
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        p.setPen(QPen(QColor(0x30,0x36,0x3d,200),1));
        p.setBrush(QColor(13,17,23,210));
        p.drawRoundedRect(rect().adjusted(0,0,-1,-1),10,10);
        if (m_monitors.isEmpty()) {
            p.setPen(QColor(0x8b,0x94,0x9e));
            p.drawText(rect(),Qt::AlignCenter,m_noMon); return;
        }
        int mnX=INT_MAX,mnY=INT_MAX,mxX=INT_MIN,mxY=INT_MIN;
        for (auto &m:m_monitors){mnX=std::min(mnX,m.x);mnY=std::min(mnY,m.y);mxX=std::max(mxX,m.x+m.width);mxY=std::max(mxY,m.y+m.height);}
        int tW=mxX-mnX,tH=mxY-mnY; if(!tW||!tH) return;
        const int P=16; int aW=width()-2*P,aH=height()-2*P;
        double sc=std::min((double)aW/tW,(double)aH/tH);
        int oX=P+(aW-(int)(tW*sc))/2,oY=P+(aH-(int)(tH*sc))/2;
        for (auto &m:m_monitors){
            int rx=oX+(int)((m.x-mnX)*sc),ry=oY+(int)((m.y-mnY)*sc);
            int rw=std::max(6,(int)(m.width*sc)),rh=std::max(6,(int)(m.height*sc));
            QRect r(rx,ry,rw,rh); bool sel=(m.name==m_selected);
            int mode=m_modes.value(m.name,-1);
            if (mode==0 && m_pixmaps.contains(m.name)) {
                // Static wallpaper — draw the image
                const QPixmap &px=m_pixmaps[m.name];
                QSize sc2=px.size().scaled(r.size(),Qt::KeepAspectRatioByExpanding);
                QPixmap sp=px.scaled(sc2,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
                int cx=(sp.width()-r.width())/2,cy=(sp.height()-r.height())/2;
                p.setClipRect(r); p.drawPixmap(r.topLeft(),sp,QRect(cx,cy,r.width(),r.height()));
                p.setClipping(false);
            } else if (mode==1) {
                // Video wallpaper
                p.fillRect(r,QColor(16,10,30));
                QFont f=p.font(); f.setPointSize(std::max(8,rh/5)); p.setFont(f);
                p.setPen(QColor(139,92,246));
                p.drawText(r,Qt::AlignCenter,"\u25b6");
            } else if (mode==2) {
                // Slideshow — draw a 2×2 grid of mini-thumbnails + shuffle badge
                p.fillRect(r, QColor(13,17,23));
                const int cols=2, rows=2;
                int cw=r.width()/cols, ch=r.height()/rows;
                // Draw thin grid lines
                p.setPen(QPen(QColor(0x30,0x36,0x3d,180),1));
                for(int ci=1;ci<cols;++ci)
                    p.drawLine(r.left()+ci*cw, r.top(), r.left()+ci*cw, r.bottom());
                for(int ri=1;ri<rows;++ri)
                    p.drawLine(r.left(), r.top()+ri*ch, r.right(), r.top()+ri*ch);
                // Small image placeholders
                p.setBrush(QColor(22,27,34));
                p.setPen(Qt::NoPen);
                for(int ci=0;ci<cols;++ci)
                    for(int ri=0;ri<rows;++ri)
                        p.drawRect(r.left()+ci*cw+1, r.top()+ri*ch+1, cw-2, ch-2);
                // Shuffle / slideshow badge (bottom-right)
                int badgeS=std::min(rh/3, 18);
                QRect badge(r.right()-badgeS-2, r.bottom()-badgeS-2, badgeS, badgeS);
                p.setBrush(QColor(0x23,0x86,0x36,220));
                p.setPen(Qt::NoPen);
                p.drawEllipse(badge);
                p.setPen(QColor(0xf0,0xf6,0xfc,230));
                QFont bf=p.font(); bf.setPointSize(std::max(6,badgeS*5/9)); bf.setBold(true); p.setFont(bf);
                p.drawText(badge, Qt::AlignCenter, "\u21ba"); // ↺
            } else {
                p.fillRect(r,QColor(22,27,34));
            }
            if(sel){p.setPen(QPen(QColor(0x58,0xa6,0xff,220),2));p.setBrush(Qt::NoBrush);p.drawRect(r);}
            else   {p.setPen(QPen(QColor(0x30,0x36,0x3d,180),1));p.setBrush(Qt::NoBrush);p.drawRect(r);}
            int lH=std::min(18,rh); QRect lr(rx,ry+rh-lH,rw,lH);
            p.fillRect(lr,QColor(0,0,0,160));
            p.setPen(sel?QColor(0x58,0xa6,0xff):QColor(0xc9,0xd1,0xd9));
            QFont nf=p.font(); nf.setPointSize(7); nf.setBold(sel); p.setFont(nf);
            p.drawText(lr,Qt::AlignCenter,m.name);
        }
    }
    void mousePressEvent(QMouseEvent *ev) override {
        if(m_monitors.isEmpty()) return;
        int mnX=INT_MAX,mnY=INT_MAX,mxX=INT_MIN,mxY=INT_MIN;
        for(auto &m:m_monitors){mnX=std::min(mnX,m.x);mnY=std::min(mnY,m.y);mxX=std::max(mxX,m.x+m.width);mxY=std::max(mxY,m.y+m.height);}
        int tW=mxX-mnX,tH=mxY-mnY; if(!tW||!tH) return;
        const int P=16; int aW=width()-2*P,aH=height()-2*P;
        double sc=std::min((double)aW/tW,(double)aH/tH);
        int oX=P+(aW-(int)(tW*sc))/2,oY=P+(aH-(int)(tH*sc))/2;
        for(auto &m:m_monitors){
            int rx=oX+(int)((m.x-mnX)*sc),ry=oY+(int)((m.y-mnY)*sc);
            int rw=std::max(6,(int)(m.width*sc)),rh=std::max(6,(int)(m.height*sc));
            if(QRect(rx,ry,rw,rh).contains(ev->pos())){emit monitorClicked(m.name);return;}
        }
    }
private:
    QList<MonitorInfo> m_monitors;
    QString m_selected, m_noMon{"No monitors"};
    QMap<QString,int>     m_modes;
    QMap<QString,QPixmap> m_pixmaps;
};
#include "MainWindow.moc"

// ============================================================
// MainWindow
// ============================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    m_s = stringsEN();
    qApp->setStyleSheet(APP_STYLE);
    ConfigManager::instance().load();
    buildUi();
    loadMonitors();
}

MonitorSlideshowState &MainWindow::slideshowState(const QString &monitor)
{
    return m_ssState[monitor];
}

void MainWindow::startSlideshowForMonitor(const QString &monitor)
{
    MonitorSlideshowState &ss = m_ssState[monitor];
    if (!ss.timer) {
        ss.timer = new QTimer(this);
        ss.timer->setSingleShot(false);
        connect(ss.timer, &QTimer::timeout, this, [this, monitor]{
            tickMonitor(monitor);
        });
    }
    ss.timer->setInterval(ss.intervalSecs * 1000);
    ss.timer->start();
    tickMonitor(monitor);
}

void MainWindow::stopSlideshowForMonitor(const QString &monitor)
{
    if (m_ssState.contains(monitor)) {
        MonitorSlideshowState &ss = m_ssState[monitor];
        if (ss.timer) ss.timer->stop();
    }
}

void MainWindow::tickMonitor(const QString &monitor)
{
    QList<GalleryItem> gallery = ConfigManager::instance().loadGallery();
    if (gallery.isEmpty()) return;
    int mode = m_ssState.contains(monitor) ? m_ssState[monitor].mediaMode : 2;
    WallpaperApplier::applySlideshowTick(monitor, gallery, mode);
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(13,17,23,218));
    p.setPen(QPen(QColor(0x30,0x36,0x3d,180),1));
    p.drawRoundedRect(rect().adjusted(1,1,-1,-1),12,12);
    QLinearGradient g(0,0,width(),0);
    g.setColorAt(0.0, QColor(0x38,0x8b,0xfd,0));
    g.setColorAt(0.2, QColor(0x58,0xa6,0xff,180));
    g.setColorAt(0.8, QColor(0x38,0x8b,0xfd,180));
    g.setColorAt(1.0, QColor(0x38,0x8b,0xfd,0));
    p.setPen(Qt::NoPen); p.setBrush(g);
    p.drawRoundedRect(QRect(0,0,width(),2),1,1);
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
}
void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton)
        move(e->globalPosition().toPoint() - m_dragPos);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == m_galleryList->viewport() && ev->type() == QEvent::MouseButtonRelease) {
        auto *me = static_cast<QMouseEvent*>(ev);
        QListWidgetItem *it = m_galleryList->itemAt(me->pos());
        if (it) {
            QRect cellRect = m_galleryList->visualItemRect(it);
            QRect xZone(cellRect.right() - 20, cellRect.top(), 20, 20);
            if (xZone.contains(me->pos())) {
                onGalleryRemove(it->data(Qt::UserRole).toString());
            } else {
                bool locked = it->data(Qt::UserRole + 2).toBool();
                if (!locked) {
                    onGalleryItemClicked(
                        it->data(Qt::UserRole).toString(),
                        it->data(Qt::UserRole + 1).toBool());
                }
            }
        }
        return true;
    }
    return QMainWindow::eventFilter(obj, ev);
}

// ============================================================
// Build UI
// ============================================================
void MainWindow::buildUi()
{
    setWindowTitle(m_s.windowTitle);
    setMinimumWidth(460);
    resize(560, 640);

    QWidget *central = new QWidget(this);
    central->setObjectName("central");
    setCentralWidget(central);
    QVBoxLayout *outerLayout = new QVBoxLayout(central);
    outerLayout->setContentsMargins(0,0,0,0);
    outerLayout->setSpacing(0);

    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget *contents = new QWidget;
    contents->setObjectName("scrollContents");
    QVBoxLayout *root = new QVBoxLayout(contents);
    root->setSpacing(8);
    root->setContentsMargins(16,12,16,12);
    root->setAlignment(Qt::AlignTop);

    scroll->setWidget(contents);
    outerLayout->addWidget(scroll);

    // ── Title bar ────────────────────────────────────────────
    {
        QHBoxLayout *tb = new QHBoxLayout;
        tb->setSpacing(6);
        QLabel *title = new QLabel("HyprWall");
        title->setStyleSheet("font-size:17px;font-weight:700;color:#c9d1d9;letter-spacing:1px;");

        m_autostartLabel = new QLabel(m_s.autostartLabel);
        m_autostartLabel->setStyleSheet("color:#8b949e;font-size:12px;");
        m_autostartSwitch = new ToggleSwitch(this);
        m_autostartSwitch->setChecked(autostartEnabled(), false);
        connect(m_autostartSwitch, &ToggleSwitch::toggled,
                this, &MainWindow::onAutostartToggle);

        m_langLabel = new QLabel(m_s.langLabel);
        m_langLabel->setStyleSheet("color:#8b949e;font-size:12px;");
        m_langCombo = new QComboBox;
        m_langCombo->addItems({"English", "\u0420\u0443\u0441\u0441\u043a\u0438\u0439"});
        m_langCombo->setFixedWidth(110);
        connect(m_langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::onLanguageChanged);

        QPushButton *closeBtn = new QPushButton("\u2715");
        closeBtn->setFixedSize(26,26);
        closeBtn->setStyleSheet(
            "QPushButton{background:transparent;border:1px solid rgba(255,70,70,50);"
            "border-radius:5px;color:#6e7681;font-size:11px;padding:0;"
            "min-height:26px;max-height:26px;}"
            "QPushButton:hover{background:rgba(218,54,51,180);border-color:transparent;color:#fff;}");
        connect(closeBtn, &QPushButton::clicked, this, &QMainWindow::close);

        tb->addWidget(title); tb->addStretch();
        tb->addWidget(m_autostartLabel);
        tb->addWidget(m_autostartSwitch);
        tb->addSpacing(10);
        tb->addWidget(m_langLabel); tb->addWidget(m_langCombo); tb->addSpacing(6);
        tb->addWidget(closeBtn);
        root->addLayout(tb);
    }

    // ── Monitor bar ──────────────────────────────────────────
    m_monitorBar = new MonitorBar(this);
    m_monitorBar->setFixedHeight(160);
    m_monitorBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_monitorBar->setNoMonitorsText(m_s.noMonitors);
    connect(m_monitorBar, &MonitorBar::monitorClicked, this, &MainWindow::onMonitorClicked);
    root->addWidget(m_monitorBar);

    // ── Settings group ────────────────────────────────────────
    m_settingsGroup = new QGroupBox(m_s.groupTitle);
    m_settingsGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QVBoxLayout *sg = new QVBoxLayout(m_settingsGroup);
    sg->setSpacing(6);
    sg->setContentsMargins(10,14,10,10);
    sg->setAlignment(Qt::AlignTop);

    // 1. Monitor info
    m_orientationLabel = new QLabel("-");
    m_orientationLabel->setObjectName("orientLabel");
    sg->addWidget(m_orientationLabel);

    // 2. Slideshow row — ToggleSwitch
    {
        QHBoxLayout *row = new QHBoxLayout;
        row->setSpacing(8);
        m_slideshowLabel = new QLabel(m_s.slideshowLabel);
        m_slideshowLabel->setStyleSheet("color:#8b949e;font-size:12px;");
        m_slideshowSwitch = new ToggleSwitch(this);
        m_slideshowSwitch->setChecked(false, false);
        connect(m_slideshowSwitch, &ToggleSwitch::toggled,
                this, &MainWindow::onSlideshowToggled);
        row->addWidget(m_slideshowLabel);
        row->addWidget(m_slideshowSwitch);
        row->addStretch();
        sg->addLayout(row);
    }

    // 3. Timer row
    m_timerRow = new QWidget;
    {
        QHBoxLayout *row = new QHBoxLayout(m_timerRow);
        row->setContentsMargins(0,0,0,0); row->setSpacing(8);
        m_intervalPrefixLbl = new QLabel(m_s.slideshowIntervalLabel);
        m_intervalCombo = new QComboBox;
        m_intervalCombo->addItems(m_s.intervalLabels);
        m_intervalCombo->setCurrentIndex(1);
        m_intervalSuffixLbl = new QLabel(m_s.slideshowMinLabel);
        connect(m_intervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this](int idx){
                    if (m_updatingControls) return;
                    if (m_currentMonitor.isEmpty()) return;
                    int secs = INTERVAL_VALUES[idx];
                    m_ssState[m_currentMonitor].intervalSecs = secs;
                    if (m_ssState[m_currentMonitor].timer &&
                        m_ssState[m_currentMonitor].timer->isActive())
                        m_ssState[m_currentMonitor].timer->setInterval(secs * 1000);
                    applyAndSaveCurrent();
                });
        row->addWidget(m_intervalPrefixLbl);
        row->addWidget(m_intervalCombo);
        row->addWidget(m_intervalSuffixLbl);
        row->addStretch();
    }
    m_timerRow->hide();
    sg->addWidget(m_timerRow);

    // 4. Media mode row
    m_mediaModeRow = new QWidget;
    {
        QHBoxLayout *row = new QHBoxLayout(m_mediaModeRow);
        row->setContentsMargins(0,0,0,0); row->setSpacing(8);
        m_mediaModeLabel = new QLabel(m_s.slideshowModeLabel);
        m_mediaModeLabel->setStyleSheet("color:#8b949e;");
        m_mediaModeCombo = new QComboBox;
        m_mediaModeCombo->addItems(m_s.slideshowModes);
        connect(m_mediaModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this](int idx){
                    if (m_updatingControls) return;
                    if (m_currentMonitor.isEmpty()) return;
                    m_ssState[m_currentMonitor].mediaMode = idx;
                    applyAndSaveCurrent();
                });
        row->addWidget(m_mediaModeLabel);
        row->addWidget(m_mediaModeCombo, 1);
    }
    m_mediaModeRow->hide();
    sg->addWidget(m_mediaModeRow);

    // 5. Fill
    m_fillRow = new QWidget;
    {
        QHBoxLayout *row = new QHBoxLayout(m_fillRow);
        row->setContentsMargins(0,0,0,0); row->setSpacing(8);
        m_fillLabel = new QLabel(m_s.fillLabel);
        m_fillLabel->setFixedWidth(90);
        m_fillLabel->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
        m_fillLabel->setStyleSheet("color:#8b949e;");
        m_fillCombo = new QComboBox;
        m_fillCombo->addItems(m_s.imgFillModes);
        connect(m_fillCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::onFillModeChanged);
        row->addWidget(m_fillLabel);
        row->addWidget(m_fillCombo, 1);
    }
    sg->addWidget(m_fillRow);

    // 6. Rotation
    m_rotRow = new QWidget;
    {
        QHBoxLayout *row = new QHBoxLayout(m_rotRow);
        row->setContentsMargins(0,0,0,0); row->setSpacing(8);
        m_rotLabel = new QLabel(m_s.rotLabel);
        m_rotLabel->setFixedWidth(90);
        m_rotLabel->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
        m_rotLabel->setStyleSheet("color:#8b949e;");
        m_rotCombo = new QComboBox;
        m_rotCombo->addItems(m_s.imgRotModes);
        connect(m_rotCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::onRotationChanged);
        row->addWidget(m_rotLabel);
        row->addWidget(m_rotCombo, 1);
    }
    sg->addWidget(m_rotRow);

    // 7. Gallery
    buildGalleryPanel(sg);

    // 8. Audio row
    m_audioRow = new QWidget;
    {
        QHBoxLayout *row = new QHBoxLayout(m_audioRow);
        row->setContentsMargins(0,0,0,0); row->setSpacing(0);
        m_audioCheck = new QCheckBox(m_s.audioCheck);
        connect(m_audioCheck, &QCheckBox::toggled, this, &MainWindow::onAudioToggled);
        row->addWidget(m_audioCheck);
        row->addStretch();
    }
    m_audioRow->hide();
    sg->addWidget(m_audioRow);

    // Volume row
    m_volumeRow = new QWidget;
    {
        QHBoxLayout *row = new QHBoxLayout(m_volumeRow);
        row->setContentsMargins(0,0,0,0); row->setSpacing(8);
        m_volumeLabelW = new QLabel(m_s.volumeLabel);
        m_volumeSlider = new QSlider(Qt::Horizontal);
        m_volumeSlider->setRange(0,100);
        m_volumeSlider->setValue(50);
        m_volumeLabel = new QLabel("50%");
        m_volumeLabel->setFixedWidth(36);
        m_volumeLabel->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
        m_volumeLabel->setStyleSheet("color:#58a6ff;font-weight:600;");
        connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
        row->addWidget(m_volumeLabelW);
        row->addWidget(m_volumeSlider, 1);
        row->addWidget(m_volumeLabel);
    }
    m_volumeRow->hide();
    sg->addWidget(m_volumeRow);

    // Bind hint
    m_bindRow = new QWidget;
    {
        QVBoxLayout *bl = new QVBoxLayout(m_bindRow);
        bl->setContentsMargins(0,4,0,0); bl->setSpacing(3);
        m_bindPrefixLabel = new QLabel(m_s.bindPrefix);
        m_bindPrefixLabel->setStyleSheet("color:#6e7681;font-size:10px;");
        m_bindHint = new QLabel;
        m_bindHint->setObjectName("bindHint");
        m_bindHint->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_bindHint->setStyleSheet(
            "color:#58a6ff;font-size:10px;font-family:monospace;"
            "background:rgba(88,166,255,8);border:1px solid rgba(88,166,255,25);"
            "border-radius:5px;padding:3px 7px;");
        bl->addWidget(m_bindPrefixLabel);
        bl->addWidget(m_bindHint);
    }
    m_bindRow->hide();
    sg->addWidget(m_bindRow);

    root->addWidget(m_settingsGroup);
}

// ============================================================
// Gallery panel  (QListWidget + IconMode)
// ============================================================
void MainWindow::buildGalleryPanel(QVBoxLayout *parent)
{
    m_galleryGroup = new QGroupBox(m_s.galleryTitle);
    m_galleryGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QVBoxLayout *vl = new QVBoxLayout(m_galleryGroup);
    vl->setSpacing(5);
    vl->setContentsMargins(8,14,8,8);

    {
        QHBoxLayout *bar = new QHBoxLayout;
        bar->setContentsMargins(0,0,0,2); bar->setSpacing(6);
        bar->addStretch();
        m_galleryAddBtn = new QPushButton(m_s.galleryAddBtn);
        m_galleryAddBtn->setObjectName("galleryAddBtn");
        m_galleryAddBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        connect(m_galleryAddBtn, &QPushButton::clicked, this, &MainWindow::onGalleryAdd);
        bar->addWidget(m_galleryAddBtn);
        vl->addLayout(bar);
    }

    m_galleryList = new QListWidget;
    m_galleryList->setObjectName("galleryList");
    m_galleryList->setViewMode(QListView::IconMode);
    m_galleryList->setIconSize(QSize(THUMB_W, THUMB_H));
    m_galleryList->setGridSize(QSize(GRID_W, GRID_H));
    m_galleryList->setResizeMode(QListView::Adjust);
    m_galleryList->setMovement(QListView::Static);
    m_galleryList->setUniformItemSizes(true);
    m_galleryList->setSpacing(3);
    m_galleryList->setWordWrap(false);
    m_galleryList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_galleryList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_galleryList->setMinimumHeight(GRID_H + 10);
    m_galleryList->setMaximumHeight(GRID_H * 3 + 30);
    m_galleryList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_galleryList->setItemDelegate(new GalleryDelegate(m_galleryList));
    m_galleryList->setSelectionMode(QAbstractItemView::NoSelection);
    m_galleryList->viewport()->installEventFilter(this);
    m_galleryList->viewport()->setObjectName("galleryViewport");

    vl->addWidget(m_galleryList);

    m_galleryEmptyLbl = new QLabel(m_s.galleryEmptyHint);
    m_galleryEmptyLbl->setAlignment(Qt::AlignCenter);
    m_galleryEmptyLbl->setObjectName("galleryEmpty");
    m_galleryEmptyLbl->hide();
    vl->addWidget(m_galleryEmptyLbl);

    parent->addWidget(m_galleryGroup);
    refreshGallery();
}

// ============================================================
// refreshGallery — populate model instantly, load thumbs async
// ============================================================
void MainWindow::refreshGallery()
{
    if (!m_galleryList) return;
    m_galleryList->clear();

    // Bump generation so any in-flight async loads for the old list are discarded
    ++m_thumbGeneration;

    bool ssLocked = !m_currentMonitor.isEmpty()
                    && m_ssState.value(m_currentMonitor).enabled;

    QList<GalleryItem> items = ConfigManager::instance().loadGallery();

    if (items.isEmpty()) {
        m_galleryList->hide();
        m_galleryEmptyLbl->show();
        return;
    }
    m_galleryEmptyLbl->hide();
    m_galleryList->show();

    for (const GalleryItem &item : items) {
        QListWidgetItem *wi = new QListWidgetItem;
        wi->setData(Qt::UserRole,     item.path);
        wi->setData(Qt::UserRole + 1, item.isVideo);
        wi->setData(Qt::UserRole + 2, ssLocked);
        wi->setText(QFileInfo(item.path).fileName());
        wi->setToolTip(item.path);
        wi->setSizeHint(QSize(GRID_W, GRID_H));
        wi->setFlags(Qt::ItemIsEnabled);

        if (item.isVideo) {
            // Video placeholder — build synchronously (trivial cost)
            QPixmap vp(THUMB_W, THUMB_H);
            vp.fill(QColor(16, 10, 30));
            QPainter pp(&vp);
            pp.setPen(QColor(139, 92, 246));
            pp.setFont(QFont("sans", THUMB_H / 4));
            pp.drawText(vp.rect(), Qt::AlignCenter, "\u25b6");
            wi->setIcon(QIcon(vp));
        } else {
            // Check cache first — no disk I/O needed if already loaded
            if (m_thumbCache.contains(item.path)) {
                wi->setIcon(QIcon(m_thumbCache[item.path]));
            } else {
                // Leave icon empty (placeholder drawn by delegate), load async
                loadThumbAsync(item.path, m_thumbGeneration);
            }
        }

        m_galleryList->addItem(wi);
    }

    // Adjust max-height to actual content rows
    int rowH = GRID_H + 6;
    int cols  = std::max(1, m_galleryList->width() / (GRID_W + 6));
    int rows  = std::max(1, (m_galleryList->count() + cols - 1) / cols);
    int maxH  = std::min(rows * rowH + 10, GRID_H * 3 + 30);
    m_galleryList->setMaximumHeight(maxH);
}

// ============================================================
// loadThumbAsync — reads + scales one image on a worker thread,
//                  then updates the matching list item on the GUI thread
// ============================================================
void MainWindow::loadThumbAsync(const QString &path, int generation)
{
    // Capture by value — safe across thread boundary
    auto *watcher = new QFutureWatcher<QPixmap>(this);

    connect(watcher, &QFutureWatcher<QPixmap>::finished, this,
            [this, path, generation, watcher]() {
                watcher->deleteLater();

                // Discard result if the gallery was refreshed again meanwhile
                if (generation != m_thumbGeneration) return;

                QPixmap px = watcher->result();
                if (px.isNull()) return;

                // Store in cache
                m_thumbCache[path] = px;

                // Find the matching item and set its icon
                for (int i = 0; i < m_galleryList->count(); ++i) {
                    QListWidgetItem *wi = m_galleryList->item(i);
                    if (wi && wi->data(Qt::UserRole).toString() == path) {
                        wi->setIcon(QIcon(px));
                        break;
                    }
                }
            });

    // The worker: load + scale entirely off the GUI thread
    QFuture<QPixmap> future = QtConcurrent::run([path]() -> QPixmap {
        QPixmap px(path);
        if (px.isNull()) return {};
        // Scale down to thumbnail size — do the heavy work here, not on GUI thread
        return px.scaled(THUMB_W, THUMB_H,
                         Qt::KeepAspectRatioByExpanding,
                         Qt::SmoothTransformation);
    });

    watcher->setFuture(future);
}

// ============================================================
// Instant apply helper
// ============================================================
void MainWindow::applyAndSaveCurrent()
{
    if (m_currentMonitor.isEmpty() || m_updatingControls) return;
    saveCurrentToPending();

    const WallpaperConfig &cfg = m_pending[m_currentMonitor];
    const MonitorSlideshowState &ss = m_ssState[m_currentMonitor];

    auto &cm = ConfigManager::instance();
    cm.setConfig(m_currentMonitor, cfg);
    cm.save();

    if (ss.enabled) {
        startSlideshowForMonitor(m_currentMonitor);
        // Show slideshow indicator regardless of filePath
        m_monitorBar->setMonitorMode(m_currentMonitor, 2, {});
    } else {
        stopSlideshowForMonitor(m_currentMonitor);
        if (!cfg.filePath.isEmpty())
            WallpaperApplier::apply(cfg);
        bool vid = WallpaperApplier::isVideoFile(cfg.filePath);
        m_monitorBar->setMonitorMode(m_currentMonitor, vid?1:0, vid?QString():cfg.filePath);
    }
}

void MainWindow::updateAutostartSwitch()
{
    if (!m_autostartSwitch) return;
    QSignalBlocker blocker(m_autostartSwitch);
    m_autostartSwitch->setChecked(autostartEnabled());
}

void MainWindow::onAutostartToggle()
{
    setAutostart(m_autostartSwitch->isChecked());
}

void MainWindow::updateSlideshowDependentWidgets(bool ssOn)
{
    m_timerRow->setVisible(ssOn);
    m_mediaModeRow->setVisible(ssOn);
    m_fillRow->setVisible(!ssOn);
    m_rotRow->setVisible(!ssOn);
    if (ssOn) {
        m_audioRow->hide();
        m_volumeRow->hide();
        m_bindRow->hide();
    }
    refreshGallery();
}

void MainWindow::switchToVideo(bool isVideo)
{
    if (m_isVideo == isVideo) return;
    m_isVideo = isVideo;
    int pf = m_fillCombo->currentIndex();
    int pr = m_rotCombo->currentIndex();
    m_fillCombo->blockSignals(true); m_rotCombo->blockSignals(true);
    m_fillCombo->clear(); m_rotCombo->clear();
    if (isVideo) { m_fillCombo->addItems(m_s.vidFillModes); m_rotCombo->addItems(m_s.vidRotModes); }
    else         { m_fillCombo->addItems(m_s.imgFillModes); m_rotCombo->addItems(m_s.imgRotModes); }
    m_fillCombo->setCurrentIndex(std::min(pf, m_fillCombo->count()-1));
    m_rotCombo->setCurrentIndex(std::min(pr, m_rotCombo->count()-1));
    m_fillCombo->blockSignals(false); m_rotCombo->blockSignals(false);
}

void MainWindow::retranslateUi()
{
    setWindowTitle(m_s.windowTitle);
    m_langLabel->setText(m_s.langLabel);
    m_monitorBar->setNoMonitorsText(m_s.noMonitors);
    m_settingsGroup->setTitle(m_s.groupTitle);
    m_audioCheck->setText(m_s.audioCheck);
    m_volumeLabelW->setText(m_s.volumeLabel);
    m_fillLabel->setText(m_s.fillLabel);
    m_rotLabel->setText(m_s.rotLabel);
    m_bindPrefixLabel->setText(m_s.bindPrefix);
    m_autostartLabel->setText(m_s.autostartLabel);
    m_galleryGroup->setTitle(m_s.galleryTitle);
    m_galleryAddBtn->setText(m_s.galleryAddBtn);
    m_galleryEmptyLbl->setText(m_s.galleryEmptyHint);
    m_slideshowLabel->setText(m_s.slideshowLabel);
    m_intervalPrefixLbl->setText(m_s.slideshowIntervalLabel);
    m_intervalSuffixLbl->setText(m_s.slideshowMinLabel);
    m_mediaModeLabel->setText(m_s.slideshowModeLabel);

    int mi = m_mediaModeCombo->currentIndex();
    m_mediaModeCombo->blockSignals(true);
    m_mediaModeCombo->clear();
    m_mediaModeCombo->addItems(m_s.slideshowModes);
    m_mediaModeCombo->setCurrentIndex(std::max(0, mi));
    m_mediaModeCombo->blockSignals(false);

    int ci = m_intervalCombo->currentIndex();
    m_intervalCombo->blockSignals(true);
    m_intervalCombo->clear();
    m_intervalCombo->addItems(m_s.intervalLabels);
    m_intervalCombo->setCurrentIndex(std::max(0, ci));
    m_intervalCombo->blockSignals(false);

    int fi = m_fillCombo->currentIndex(), ri = m_rotCombo->currentIndex();
    m_fillCombo->blockSignals(true); m_rotCombo->blockSignals(false);
    m_fillCombo->clear(); m_rotCombo->clear();
    if (m_isVideo) { m_fillCombo->addItems(m_s.vidFillModes); m_rotCombo->addItems(m_s.vidRotModes); }
    else           { m_fillCombo->addItems(m_s.imgFillModes); m_rotCombo->addItems(m_s.imgRotModes); }
    m_fillCombo->setCurrentIndex(std::min(fi, m_fillCombo->count()-1));
    m_rotCombo->setCurrentIndex(std::min(ri, m_rotCombo->count()-1));
    m_fillCombo->blockSignals(false); m_rotCombo->blockSignals(false);

    if (!m_currentMonitor.isEmpty()) populateSettings(m_currentMonitor);
    m_monitorBar->update();
}

void MainWindow::onLanguageChanged(int idx)
{
    m_isRU = (idx == 1);
    m_s = m_isRU ? stringsRU() : stringsEN();
    retranslateUi();
}

QString MainWindow::bindString() const
{
    return QString("bind = SUPER, F9, exec, hyprwall --toggle-audio %1").arg(m_currentMonitor);
}

QString MainWindow::smartBrowseDir() const
{
    for (const QString &d : {
            QDir::homePath()+"/Pictures/wallpapers",
            QDir::homePath()+"/Pictures/wallpaper",
            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
            QDir::homePath() })
        if (QDir(d).exists()) return d;
    return QDir::homePath();
}

void MainWindow::loadMonitors()
{
    m_monitors = MonitorDetector::detect();
    m_monitorBar->setMonitors(m_monitors);
    ConfigManager &cm = ConfigManager::instance();
    for (const MonitorInfo &m : m_monitors) {
        WallpaperConfig cfg = cm.getConfig(m.name);
        cfg.monitorName = m.name;
        m_pending[m.name] = cfg;
        MonitorSlideshowState &ss = m_ssState[m.name];
        ss.enabled      = cfg.slideshowEnabled;
        ss.intervalSecs = cfg.slideshowInterval;
        ss.mediaMode    = cfg.slideshowMode;
        if (ss.enabled) {
            startSlideshowForMonitor(m.name);
            m_monitorBar->setMonitorMode(m.name, 2, {});
        } else if (!cfg.filePath.isEmpty()) {
            bool vid = WallpaperApplier::isVideoFile(cfg.filePath);
            m_monitorBar->setMonitorMode(m.name, vid?1:0, vid?QString():cfg.filePath);
        }
    }
    if (!m_monitors.isEmpty()) onMonitorClicked(m_monitors.first().name);
}

void MainWindow::onMonitorClicked(const QString &name)
{
    auto it = std::find_if(m_monitors.cbegin(), m_monitors.cend(),
        [&](const MonitorInfo &m){ return m.name == name; });
    if (it == m_monitors.cend()) return;
    m_currentMonitor = name;
    m_monitorBar->setSelected(name);
    populateSettings(name);
}

void MainWindow::populateSettings(const QString &monitorName)
{
    m_updatingControls = true;

    auto it = std::find_if(m_monitors.cbegin(), m_monitors.cend(),
        [&](const MonitorInfo &m){ return m.name == monitorName; });
    if (it != m_monitors.cend())
        m_orientationLabel->setText(
            QString("%1  |  %2x%3  @  %4Hz  scale %5")
            .arg(orientStr(it->transform, m_s))
            .arg(it->width).arg(it->height).arg(it->refreshRate)
            .arg(it->scale, 0, 'f', 2));

    WallpaperConfig cfg = m_pending.contains(monitorName)
        ? m_pending[monitorName]
        : ConfigManager::instance().getConfig(monitorName);
    cfg.monitorName = monitorName;

    const MonitorSlideshowState &ss = m_ssState.contains(monitorName)
        ? m_ssState[monitorName] : MonitorSlideshowState{};

    {
        QSignalBlocker b(m_slideshowSwitch);
        m_slideshowSwitch->setChecked(ss.enabled, false);
    }

    int sIdx = 1;
    for (int i = 0; i < 6; ++i)
        if (INTERVAL_VALUES[i] == ss.intervalSecs) { sIdx = i; break; }
    m_intervalCombo->setCurrentIndex(sIdx);
    m_mediaModeCombo->setCurrentIndex(std::max(0, std::min(ss.mediaMode, m_mediaModeCombo->count()-1)));

    bool isVid = !cfg.filePath.isEmpty() && WallpaperApplier::isVideoFile(cfg.filePath);
    m_isVideo = !isVid;
    switchToVideo(isVid);

    m_fillCombo->blockSignals(true); m_rotCombo->blockSignals(true);
    m_fillCombo->setCurrentIndex(static_cast<int>(cfg.fillMode));
    m_rotCombo->setCurrentIndex(static_cast<int>(cfg.rotation));
    m_audioCheck->setChecked(cfg.audioEnabled);
    m_volumeSlider->setValue(cfg.audioVolume);
    m_volumeLabel->setText(QString("%1%").arg(cfg.audioVolume));
    m_fillCombo->blockSignals(false); m_rotCombo->blockSignals(false);

    updateSlideshowDependentWidgets(ss.enabled);
    if (!ss.enabled) {
        m_audioRow->setVisible(isVid);
        m_volumeRow->setVisible(isVid && cfg.audioEnabled);
        if (isVid) { m_bindHint->setText(bindString()); m_bindRow->show(); }
        else m_bindRow->hide();
    }

    // Show correct indicator: slideshow=2, video=1, image=0
    if (ss.enabled)
        m_monitorBar->setMonitorMode(monitorName, 2, {});
    else
        m_monitorBar->setMonitorMode(monitorName, isVid?1:0, isVid?QString():cfg.filePath);

    m_updatingControls = false;
}

void MainWindow::saveCurrentToPending()
{
    if (m_currentMonitor.isEmpty()) return;
    WallpaperConfig cfg;
    cfg.monitorName  = m_currentMonitor;
    cfg.filePath     = m_pending.contains(m_currentMonitor)
                       ? m_pending[m_currentMonitor].filePath : QString();
    cfg.fillMode     = static_cast<FillMode>(m_fillCombo->currentIndex());
    cfg.rotation     = static_cast<WallpaperRotation>(m_rotCombo->currentIndex());
    cfg.audioEnabled = m_audioCheck->isChecked();
    cfg.audioVolume  = m_volumeSlider->value();
    const MonitorSlideshowState &ss = m_ssState[m_currentMonitor];
    cfg.slideshowEnabled  = ss.enabled;
    cfg.slideshowInterval = ss.intervalSecs;
    cfg.slideshowMode     = ss.mediaMode;
    m_pending[m_currentMonitor] = cfg;
}

void MainWindow::onApplyAll()
{
    saveCurrentToPending();
    auto &cm = ConfigManager::instance();
    for (auto it = m_pending.cbegin(); it != m_pending.cend(); ++it)
        cm.setConfig(it.key(), it.value());
    cm.save();

    QMap<QString, WallpaperConfig> staticConfigs;
    for (auto it = m_pending.cbegin(); it != m_pending.cend(); ++it) {
        const MonitorSlideshowState &ss = m_ssState.value(it.key());
        if (!ss.enabled)
            staticConfigs[it.key()] = it.value();
    }
    if (!staticConfigs.isEmpty())
        WallpaperApplier::applyAll(staticConfigs);

    for (auto it = m_pending.cbegin(); it != m_pending.cend(); ++it) {
        const QString &mon = it.key();
        MonitorSlideshowState &ss = m_ssState[mon];
        if (ss.enabled) startSlideshowForMonitor(mon);
        else stopSlideshowForMonitor(mon);
    }

    for (auto it = m_pending.cbegin(); it != m_pending.cend(); ++it) {
        const QString &mon = it.key();
        const MonitorSlideshowState &ss = m_ssState.value(mon);
        if (ss.enabled) {
            m_monitorBar->setMonitorMode(mon, 2, {});
        } else {
            bool vid = WallpaperApplier::isVideoFile(it.value().filePath);
            m_monitorBar->setMonitorMode(mon, vid?1:0,
                                         vid?QString():it.value().filePath);
        }
    }
}

void MainWindow::onGalleryAdd()
{
    QString title  = m_isRU ? QString("\u0414\u043e\u0431\u0430\u0432\u0438\u0442\u044c \u0432 \u0433\u0430\u043b\u0435\u0440\u0435\u044e")
                             : QString("Add to gallery");
    QString filter = m_isRU ? QString("\u0424\u043e\u0442\u043e \u0438 \u0432\u0438\u0434\u0435\u043e")
                             : QString("Images and video");
    filter += " (*.jpg *.jpeg *.png *.bmp *.webp *.gif *.mp4 *.mkv *.avi *.webm *.mov);;";
    filter += m_isRU ? QString("\u0412\u0441\u0435 \u0444\u0430\u0439\u043b\u044b (*)")
                     : QString("All files (*)");
    QStringList paths = QFileDialog::getOpenFileNames(this, title, smartBrowseDir(), filter);
    if (paths.isEmpty()) return;
    ConfigManager::instance().addToGallery(paths);
    refreshGallery();
}

void MainWindow::onGalleryRemove(const QString &path)
{
    // Evict from cache when item is deleted
    m_thumbCache.remove(path);
    ConfigManager::instance().removeFromGallery(path);
    refreshGallery();
    if (m_pending.contains(m_currentMonitor) &&
        m_pending[m_currentMonitor].filePath == path) {
        m_pending[m_currentMonitor].filePath.clear();
        m_monitorBar->setMonitorMode(m_currentMonitor, -1, {});
    }
}

void MainWindow::onGalleryItemClicked(const QString &path, bool isVideo)
{
    if (m_currentMonitor.isEmpty()) return;
    if (!m_pending.contains(m_currentMonitor)) {
        WallpaperConfig cfg; cfg.monitorName = m_currentMonitor;
        m_pending[m_currentMonitor] = cfg;
    }
    m_pending[m_currentMonitor].filePath = path;
    switchToVideo(isVideo);
    bool ssOn = m_ssState.value(m_currentMonitor).enabled;
    m_audioRow->setVisible(isVideo && !ssOn);
    m_volumeRow->setVisible(isVideo && m_audioCheck->isChecked() && !ssOn);
    if (isVideo && !ssOn) { m_bindHint->setText(bindString()); m_bindRow->show(); }
    else m_bindRow->hide();
    // When slideshow is on, item clicks don't change the bar indicator
    if (!ssOn)
        m_monitorBar->setMonitorMode(m_currentMonitor, isVideo?1:0,
                                     isVideo?QString():path);
    applyAndSaveCurrent();
}

void MainWindow::onSlideshowToggled(bool checked)
{
    if (m_updatingControls) return;
    if (m_currentMonitor.isEmpty()) return;
    MonitorSlideshowState &ss = m_ssState[m_currentMonitor];
    ss.enabled      = checked;
    ss.intervalSecs = INTERVAL_VALUES[m_intervalCombo->currentIndex()];
    ss.mediaMode    = m_mediaModeCombo->currentIndex();
    updateSlideshowDependentWidgets(checked);
    applyAndSaveCurrent();
    if (!checked) {
        bool isVid = m_pending.contains(m_currentMonitor) &&
                     WallpaperApplier::isVideoFile(m_pending[m_currentMonitor].filePath);
        m_audioRow->setVisible(isVid);
        m_volumeRow->setVisible(isVid && m_audioCheck->isChecked());
        if (isVid) { m_bindHint->setText(bindString()); m_bindRow->show(); }
        else m_bindRow->hide();
    }
}

void MainWindow::onFillModeChanged(int)
{
    applyAndSaveCurrent();
}

void MainWindow::onRotationChanged(int)
{
    applyAndSaveCurrent();
}

void MainWindow::onAudioToggled(bool checked)
{
    m_volumeRow->setVisible(checked);
    applyAndSaveCurrent();
}

void MainWindow::onVolumeChanged(int val)
{
    m_volumeLabel->setText(QString("%1%").arg(val));
    applyAndSaveCurrent();
}
