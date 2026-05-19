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
#include <QMap>
#include <QTextStream>
#include <QFont>
#include <QSizePolicy>
#include <algorithm>
#include <climits>

// ---- Interval table (seconds) ----
const int MainWindow::INTERVAL_VALUES[] = { 60, 300, 600, 900, 1800, 3600 };

// ============================================================
// App stylesheet (original design with transparency)
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
QPushButton#applyBtn {
    background: #238636; border: 1px solid rgba(240,246,252,0.1);
    border-radius: 8px; color: #fff; font-weight: 600; font-size: 14px;
    min-height: 40px; max-height: 40px; padding: 0; letter-spacing: 0.5px;
}
QPushButton#applyBtn:hover { background: #2ea043; }
QPushButton#applyBtn:pressed { background: #238636; }
QPushButton#autostartEnableBtn {
    background: rgba(35,134,54,40); border: 1px solid rgba(35,134,54,120);
    border-radius: 6px; color: #3fb950; font-weight: 500;
    padding: 0 12px; min-height: 28px; max-height: 28px;
}
QPushButton#autostartEnableBtn:hover { background: rgba(35,134,54,80); border-color: #3fb950; }
QPushButton#autostartDisableBtn {
    background: rgba(218,54,51,30); border: 1px solid rgba(218,54,51,100);
    border-radius: 6px; color: #f85149; font-weight: 500;
    padding: 0 12px; min-height: 28px; max-height: 28px;
}
QPushButton#autostartDisableBtn:hover { background: rgba(218,54,51,70); border-color: #f85149; }
QPushButton#galleryAddBtn {
    background: rgba(56,139,253,20); border: 1px solid rgba(56,139,253,80);
    border-radius: 6px; color: #58a6ff; font-weight: 500;
    padding: 0 14px; min-height: 28px; max-height: 28px;
}
QPushButton#galleryAddBtn:hover { background: rgba(56,139,253,50); border-color: #58a6ff; }
QPushButton#thumbRemove {
    background: rgba(218,54,51,160); border: none;
    border-radius: 9px; color: #fff; font-weight: 700;
    font-size: 11px; padding: 0;
    min-height: 18px; max-height: 18px;
    min-width: 18px; max-width: 18px;
}
QPushButton#thumbRemove:hover { background: rgba(218,54,51,230); }
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
QWidget#galleryThumb {
    background: rgba(22,27,34,200);
    border: 1px solid #30363d;
    border-radius: 6px;
}
QWidget#galleryThumb:hover { border-color: #58a6ff; }
)";

// ---- Helpers ----
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
                const QPixmap &px=m_pixmaps[m.name];
                QSize sc2=px.size().scaled(r.size(),Qt::KeepAspectRatioByExpanding);
                QPixmap sp=px.scaled(sc2,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
                int cx=(sp.width()-r.width())/2,cy=(sp.height()-r.height())/2;
                p.setClipRect(r);
                p.drawPixmap(r.topLeft(),sp,QRect(cx,cy,r.width(),r.height()));
                p.setClipping(false);
            } else if (mode==1) {
                p.fillRect(r,QColor(16,10,30));
                QFont f=p.font(); f.setPointSize(std::max(8,rh/5)); p.setFont(f);
                p.setPen(QColor(139,92,246));
                p.drawText(r,Qt::AlignCenter,"\u25b6");
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

// ---- Gallery thumbnail ----
static QWidget* makeThumb(const GalleryItem &item, MainWindow *mw,
                           const QString &removeTooltip)
{
    QWidget *w = new QWidget;
    w->setFixedSize(100, 80);
    w->setCursor(Qt::PointingHandCursor);
    w->setToolTip(QFileInfo(item.path).fileName());
    w->setObjectName("galleryThumb");
    w->setProperty("itemPath",    item.path);
    w->setProperty("itemIsVideo", item.isVideo);

    QLabel *img = new QLabel(w);
    img->setFixedSize(100, 80);
    img->setAlignment(Qt::AlignCenter);
    img->setObjectName("thumbImg");
    img->setProperty("itemPath",    item.path);
    img->setProperty("itemIsVideo", item.isVideo);

    if (!item.isVideo) {
        QPixmap px(item.path);
        if (!px.isNull())
            img->setPixmap(px.scaled(100, 80,
                Qt::KeepAspectRatioByExpanding,
                Qt::SmoothTransformation).copy(0,0,100,80));
        else
            img->setText(QFileInfo(item.path).suffix().toUpper());
    } else {
        img->setText("\u25b6 " + QFileInfo(item.path).suffix().toUpper());
        img->setStyleSheet("background:rgba(20,10,40,200); color:#a78bfa; font-size:12px; font-weight:600;");
    }

    QPushButton *del = new QPushButton("\u00d7", w);
    del->setFixedSize(18, 18);
    del->move(82, 0);
    del->setObjectName("thumbRemove");
    del->setToolTip(removeTooltip);
    QObject::connect(del, &QPushButton::clicked, [mw, path=item.path]{
        mw->onGalleryRemove(path);
    });

    img->installEventFilter(mw);
    return w;
}

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

    m_slideshowTimer = new QTimer(this);
    connect(m_slideshowTimer, &QTimer::timeout, this, &MainWindow::onSlideshowTick);

    ConfigManager::instance().load();
    buildUi();
    loadMonitors();

    // restore slideshow state
    SlideshowConfig ss = ConfigManager::instance().slideshowConfig();
    m_slideshowCheck->setChecked(ss.enabled);
    int sIdx = 1;
    for (int i = 0; i < 6; ++i)
        if (INTERVAL_VALUES[i] == ss.intervalSecs) { sIdx = i; break; }
    m_intervalCombo->setCurrentIndex(sIdx);
    m_intervalCombo->setEnabled(ss.enabled);
    if (ss.enabled)
        m_slideshowTimer->start(ss.intervalSecs * 1000);
    updateSlideshowDependentWidgets(ss.enabled);
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
    if (ev->type() == QEvent::MouseButtonRelease) {
        QLabel *lbl = qobject_cast<QLabel*>(obj);
        if (lbl && lbl->objectName() == "thumbImg") {
            onGalleryItemClicked(lbl->property("itemPath").toString(),
                                 lbl->property("itemIsVideo").toBool());
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, ev);
}

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

    // ── Title bar ──────────────────────────────────────────────
    {
        QHBoxLayout *tb = new QHBoxLayout;
        tb->setSpacing(6);
        QLabel *title = new QLabel("HyprWall");
        title->setStyleSheet("font-size:17px;font-weight:700;color:#c9d1d9;letter-spacing:1px;");

        m_autostartLabel = new QLabel(m_s.autostartLabel);
        m_autostartLabel->setStyleSheet("color:#8b949e;font-size:12px;");
        m_autostartBtn = new QPushButton;
        m_autostartBtn->setFixedWidth(90);
        updateAutostartButton();
        connect(m_autostartBtn, &QPushButton::clicked, this, &MainWindow::onAutostartToggle);

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
        tb->addWidget(m_autostartLabel); tb->addWidget(m_autostartBtn); tb->addSpacing(10);
        tb->addWidget(m_langLabel); tb->addWidget(m_langCombo); tb->addSpacing(6);
        tb->addWidget(closeBtn);
        root->addLayout(tb);
    }

    // ── Monitor bar ────────────────────────────────────────────
    m_monitorBar = new MonitorBar(this);
    m_monitorBar->setFixedHeight(160);
    m_monitorBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_monitorBar->setNoMonitorsText(m_s.noMonitors);
    connect(m_monitorBar, &MonitorBar::monitorClicked, this, &MainWindow::onMonitorClicked);
    root->addWidget(m_monitorBar);

    // ── Settings group ─────────────────────────────────────────
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

    // 2. Slideshow / static toggle
    {
        QHBoxLayout *row = new QHBoxLayout;
        row->setSpacing(8);
        m_slideshowCheck = new QCheckBox(m_s.slideshowLabel);
        connect(m_slideshowCheck, &QCheckBox::toggled, this, &MainWindow::onSlideshowToggled);
        row->addWidget(m_slideshowCheck);
        row->addStretch();
        sg->addLayout(row);
    }

    // 3. Timer (visible when slideshow on)
    m_timerRow = new QWidget;
    {
        QHBoxLayout *row = new QHBoxLayout(m_timerRow);
        row->setContentsMargins(0,0,0,0); row->setSpacing(8);
        m_intervalPrefixLbl = new QLabel(m_s.slideshowIntervalLabel);
        m_intervalCombo = new QComboBox;
        m_intervalCombo->addItems(m_s.intervalLabels);
        m_intervalCombo->setCurrentIndex(1);
        m_intervalCombo->setEnabled(false);
        m_intervalSuffixLbl = new QLabel(m_s.slideshowMinLabel);
        connect(m_intervalCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this](int idx){
                    if (m_slideshowCheck->isChecked()) {
                        int secs = INTERVAL_VALUES[idx];
                        m_slideshowTimer->setInterval(secs * 1000);
                        SlideshowConfig ss = ConfigManager::instance().slideshowConfig();
                        ss.intervalSecs = secs;
                        ConfigManager::instance().setSlideshowConfig(ss);
                        ConfigManager::instance().save();
                    }
                });
        row->addWidget(m_intervalPrefixLbl);
        row->addWidget(m_intervalCombo);
        row->addWidget(m_intervalSuffixLbl);
        row->addStretch();
    }
    m_timerRow->hide();
    sg->addWidget(m_timerRow);

    // 4. Media mode (photo / video / both) — visible when slideshow on
    m_mediaModeRow = new QWidget;
    {
        QHBoxLayout *row = new QHBoxLayout(m_mediaModeRow);
        row->setContentsMargins(0,0,0,0); row->setSpacing(8);
        QLabel *lbl = new QLabel(m_s.slideshowModeLabel);
        lbl->setStyleSheet("color:#8b949e;");
        m_mediaModeCombo = new QComboBox;
        m_mediaModeCombo->addItems(m_s.slideshowModes);
        row->addWidget(lbl);
        row->addWidget(m_mediaModeCombo, 1);
    }
    m_mediaModeRow->hide();
    sg->addWidget(m_mediaModeRow);

    // 5. Gallery
    buildGalleryPanel(sg);

    // 6. Audio row (visible for video, not slideshow)
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

    // 7. Fill (hidden when slideshow)
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
        row->addWidget(m_fillLabel);
        row->addWidget(m_fillCombo, 1);
    }
    sg->addWidget(m_fillRow);

    // 8. Rotation (hidden when slideshow)
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
        row->addWidget(m_rotLabel);
        row->addWidget(m_rotCombo, 1);
    }
    sg->addWidget(m_rotRow);

    // bind hint (video)
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

    // 9. Apply
    m_applyBtn = new QPushButton(m_s.applyBtn);
    m_applyBtn->setObjectName("applyBtn");
    connect(m_applyBtn, &QPushButton::clicked, this, &MainWindow::onApplyAll);
    sg->addWidget(m_applyBtn);

    root->addWidget(m_settingsGroup);
}

void MainWindow::buildGalleryPanel(QVBoxLayout *parent)
{
    m_galleryGroup = new QGroupBox(m_s.galleryTitle);
    QVBoxLayout *vl = new QVBoxLayout(m_galleryGroup);
    vl->setSpacing(6);
    vl->setContentsMargins(8,14,8,8);

    // Add button
    {
        QHBoxLayout *bar = new QHBoxLayout;
        m_galleryAddBtn = new QPushButton(m_s.galleryAddBtn);
        m_galleryAddBtn->setObjectName("galleryAddBtn");
        connect(m_galleryAddBtn, &QPushButton::clicked, this, &MainWindow::onGalleryAdd);
        bar->addStretch();
        bar->addWidget(m_galleryAddBtn);
        vl->addLayout(bar);
    }

    // Scroll area with grid
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFixedHeight(190);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("background:transparent;border:none;");

    m_galleryGrid = new QWidget;
    m_galleryGrid->setObjectName("galleryGrid");
    scroll->setWidget(m_galleryGrid);

    m_galleryEmptyLbl = new QLabel(m_s.galleryEmptyHint);
    m_galleryEmptyLbl->setAlignment(Qt::AlignCenter);
    m_galleryEmptyLbl->setObjectName("galleryEmpty");

    vl->addWidget(scroll);
    parent->addWidget(m_galleryGroup);

    refreshGallery();
}

void MainWindow::refreshGallery()
{
    QLayout *old = m_galleryGrid->layout();
    if (old) {
        QLayoutItem *item;
        while ((item = old->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        delete old;
    }

    QList<GalleryItem> items = ConfigManager::instance().loadGallery();
    if (items.isEmpty()) {
        QVBoxLayout *vl = new QVBoxLayout(m_galleryGrid);
        vl->setAlignment(Qt::AlignCenter);
        vl->addWidget(m_galleryEmptyLbl);
        m_galleryEmptyLbl->show();
        return;
    }
    m_galleryEmptyLbl->hide();

    QGridLayout *grid = new QGridLayout(m_galleryGrid);
    grid->setSpacing(4);
    grid->setContentsMargins(4,4,4,4);
    grid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    const int COLS = 4;
    int col = 0, row = 0;
    for (const GalleryItem &item : items) {
        QWidget *thumb = makeThumb(item, this, m_s.galleryRemoveTooltip);
        grid->addWidget(thumb, row, col);
        if (++col >= COLS) { col = 0; ++row; }
    }
}

void MainWindow::updateAutostartButton()
{
    if (!m_autostartBtn) return;
    if (autostartEnabled()) {
        m_autostartBtn->setText(m_s.autostartDisable);
        m_autostartBtn->setObjectName("autostartDisableBtn");
    } else {
        m_autostartBtn->setText(m_s.autostartEnable);
        m_autostartBtn->setObjectName("autostartEnableBtn");
    }
    m_autostartBtn->style()->unpolish(m_autostartBtn);
    m_autostartBtn->style()->polish(m_autostartBtn);
}

void MainWindow::onAutostartToggle()
{
    setAutostart(!autostartEnabled());
    updateAutostartButton();
}

void MainWindow::updateSlideshowDependentWidgets(bool ssOn)
{
    m_timerRow->setVisible(ssOn);
    m_mediaModeRow->setVisible(ssOn);
    m_fillRow->setVisible(!ssOn);
    m_rotRow->setVisible(!ssOn);
    // audio only relevant for single-file video, not slideshow
    if (ssOn) {
        m_audioRow->hide();
        m_volumeRow->hide();
        m_bindRow->hide();
    }
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
    m_applyBtn->setText(m_s.applyBtn);
    m_bindPrefixLabel->setText(m_s.bindPrefix);
    m_autostartLabel->setText(m_s.autostartLabel);
    m_galleryGroup->setTitle(m_s.galleryTitle);
    m_galleryAddBtn->setText(m_s.galleryAddBtn);
    m_galleryEmptyLbl->setText(m_s.galleryEmptyHint);
    m_slideshowCheck->setText(m_s.slideshowLabel);
    m_intervalPrefixLbl->setText(m_s.slideshowIntervalLabel);
    m_intervalSuffixLbl->setText(m_s.slideshowMinLabel);
    updateAutostartButton();
    int ci = m_intervalCombo->currentIndex();
    m_intervalCombo->blockSignals(true);
    m_intervalCombo->clear();
    m_intervalCombo->addItems(m_s.intervalLabels);
    m_intervalCombo->setCurrentIndex(ci);
    m_intervalCombo->blockSignals(false);
    int fi = m_fillCombo->currentIndex(), ri = m_rotCombo->currentIndex();
    m_fillCombo->blockSignals(true); m_rotCombo->blockSignals(true);
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
        if (!cfg.filePath.isEmpty()) {
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
    saveCurrentToPending();
    m_currentMonitor = name;
    m_monitorBar->setSelected(name);
    populateSettings(name);
}

void MainWindow::populateSettings(const QString &monitorName)
{
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

    bool isVid = WallpaperApplier::isVideoFile(cfg.filePath);
    m_isVideo = !isVid;
    switchToVideo(isVid);

    m_fillCombo->blockSignals(true); m_rotCombo->blockSignals(true);
    m_fillCombo->setCurrentIndex(static_cast<int>(cfg.fillMode));
    m_rotCombo->setCurrentIndex(static_cast<int>(cfg.rotation));
    m_audioCheck->setChecked(cfg.audioEnabled);
    m_volumeSlider->setValue(cfg.audioVolume);
    m_volumeLabel->setText(QString("%1%").arg(cfg.audioVolume));
    m_fillCombo->blockSignals(false); m_rotCombo->blockSignals(false);

    bool ssOn = m_slideshowCheck->isChecked();
    m_audioRow->setVisible(isVid && !ssOn);
    m_volumeRow->setVisible(isVid && cfg.audioEnabled && !ssOn);
    if (isVid && !ssOn) { m_bindHint->setText(bindString()); m_bindRow->show(); }
    else m_bindRow->hide();

    m_monitorBar->setMonitorMode(monitorName, isVid?1:0, isVid?QString():cfg.filePath);
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
    m_pending[m_currentMonitor] = cfg;
}

void MainWindow::onApplyAll()
{
    saveCurrentToPending();
    auto &cm = ConfigManager::instance();
    for (auto it = m_pending.cbegin(); it != m_pending.cend(); ++it)
        cm.setConfig(it.key(), it.value());
    cm.save();
    WallpaperApplier::applyAll(m_pending);
    for (auto it = m_pending.cbegin(); it != m_pending.cend(); ++it) {
        bool vid = WallpaperApplier::isVideoFile(it.value().filePath);
        m_monitorBar->setMonitorMode(it.key(), vid?1:0,
                                     vid?QString():it.value().filePath);
    }
}

void MainWindow::onGalleryAdd()
{
    QString title  = m_isRU ? QString("\u0414\u043e\u0431\u0430\u0432\u0438\u0442\u044c \u0432 \u0433\u0430\u043b\u0435\u0440\u0435\u044e") : QString("Add to gallery");
    QString filter = m_isRU ? QString("\u0424\u043e\u0442\u043e \u0438 \u0432\u0438\u0434\u0435\u043e") : QString("Images and video");
    filter += " (*.jpg *.jpeg *.png *.bmp *.webp *.gif *.mp4 *.mkv *.avi *.webm *.mov);;";
    filter += m_isRU ? QString("\u0412\u0441\u0435 \u0444\u0430\u0439\u043b\u044b (*)") : QString("All files (*)");
    QStringList paths = QFileDialog::getOpenFileNames(this, title, smartBrowseDir(), filter);
    if (paths.isEmpty()) return;
    ConfigManager::instance().addToGallery(paths);
    refreshGallery();
}

void MainWindow::onGalleryRemove(const QString &path)
{
    ConfigManager::instance().removeFromGallery(path);
    refreshGallery();
    // if removed file was pending for current monitor — clear it
    if (m_pending.contains(m_currentMonitor) &&
        m_pending[m_currentMonitor].filePath == path) {
        m_pending[m_currentMonitor].filePath.clear();
        m_monitorBar->setMonitorMode(m_currentMonitor, -1, {});
    }
}

void MainWindow::onGalleryItemClicked(const QString &path, bool isVideo)
{
    if (m_currentMonitor.isEmpty()) return;
    // store chosen path into pending
    if (!m_pending.contains(m_currentMonitor)) {
        WallpaperConfig cfg;
        cfg.monitorName = m_currentMonitor;
        m_pending[m_currentMonitor] = cfg;
    }
    m_pending[m_currentMonitor].filePath = path;

    switchToVideo(isVideo);
    bool ssOn = m_slideshowCheck->isChecked();
    m_audioRow->setVisible(isVideo && !ssOn);
    m_volumeRow->setVisible(isVideo && m_audioCheck->isChecked() && !ssOn);
    if (isVideo && !ssOn) { m_bindHint->setText(bindString()); m_bindRow->show(); }
    else m_bindRow->hide();
    m_monitorBar->setMonitorMode(m_currentMonitor, isVideo?1:0,
                                 isVideo?QString():path);
}

void MainWindow::onSlideshowToggled(bool checked)
{
    m_intervalCombo->setEnabled(checked);
    updateSlideshowDependentWidgets(checked);

    SlideshowConfig ss = ConfigManager::instance().slideshowConfig();
    ss.enabled = checked;
    ss.intervalSecs = INTERVAL_VALUES[m_intervalCombo->currentIndex()];
    ConfigManager::instance().setSlideshowConfig(ss);
    ConfigManager::instance().save();

    if (checked)
        m_slideshowTimer->start(ss.intervalSecs * 1000);
    else
        m_slideshowTimer->stop();
}

void MainWindow::onSlideshowTick()
{
    QList<GalleryItem> gallery = ConfigManager::instance().loadGallery();
    if (gallery.isEmpty()) return;
    WallpaperApplier::applySlideshowRandom(m_monitors, gallery);
}

void MainWindow::onFillModeChanged(int) {}
void MainWindow::onRotationChanged(int) {}

void MainWindow::onAudioToggled(bool checked)
{
    m_volumeRow->setVisible(checked);
}

void MainWindow::onVolumeChanged(int val)
{
    m_volumeLabel->setText(QString("%1%").arg(val));
}
