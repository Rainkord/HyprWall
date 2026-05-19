#include "MainWindow.h"
#include "WallpaperApplier.h"
#include "ConfigManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLabel>
#include <QPainter>
#include <QLinearGradient>
#include <QApplication>
#include <QMessageBox>
#include <QFont>
#include <QDir>
#include <QMouseEvent>
#include <QMap>
#include <QPixmap>
#include <QProcess>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QRadioButton>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QSizePolicy>
#include <QScrollArea>
#include <algorithm>
#include <climits>

const int MainWindow::INTERVAL_SECS[4] = { 600, 1200, 1800, 3600 };

static const int LABEL_W = 90;
static const int ROW_H   = 32;
static const int BTN_W   = 80;

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
QWidget#modeBar {
    background: rgba(13,17,23,160);
    border: 1px solid #30363d;
    border-radius: 7px;
}
QRadioButton {
    spacing: 6px;
    color: #8b949e;
    padding: 4px 10px;
    border-radius: 5px;
}
QRadioButton:checked { color: #c9d1d9; background: rgba(88,166,255,18); }
QRadioButton::indicator {
    width: 13px; height: 13px;
    border: 1px solid #30363d;
    border-radius: 7px;
    background: rgba(13,17,23,200);
}
QRadioButton::indicator:checked { background: #58a6ff; border-color: #58a6ff; }
QLineEdit {
    background: rgba(13,17,23,200);
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 0 10px;
    color: #c9d1d9;
    selection-background-color: #388bfd;
    selection-color: #fff;
    min-height: 32px; max-height: 32px;
}
QLineEdit:focus { border: 1px solid #58a6ff; background: rgba(13,17,23,230); }
QComboBox {
    background: rgba(22,27,34,220);
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 0 10px;
    color: #c9d1d9;
    min-height: 32px; max-height: 32px;
}
QComboBox:hover { border: 1px solid #58a6ff; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox::down-arrow {
    image: none;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-top: 5px solid #8b949e;
    width: 0; height: 0;
    margin-right: 8px;
}
QComboBox QAbstractItemView {
    background: #161b22;
    border: 1px solid #30363d;
    border-radius: 6px;
    selection-background-color: rgba(88,166,255,40);
    color: #c9d1d9;
    outline: none;
    padding: 2px;
}
QPushButton {
    background: rgba(48,54,61,220);
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 0 16px;
    color: #c9d1d9;
    font-weight: 500;
    min-height: 32px; max-height: 32px;
}
QPushButton:hover { background: rgba(56,139,253,30); border: 1px solid #388bfd; color: #58a6ff; }
QPushButton:pressed { background: rgba(56,139,253,20); }
QPushButton#applyBtn {
    background: #238636;
    border: 1px solid rgba(240,246,252,0.1);
    border-radius: 8px;
    color: #fff;
    font-weight: 600;
    font-size: 14px;
    min-height: 40px; max-height: 40px;
    padding: 0;
    letter-spacing: 0.5px;
}
QPushButton#applyBtn:hover { background: #2ea043; }
QPushButton#applyBtn:pressed { background: #238636; }
QPushButton#autostartEnableBtn {
    background: rgba(35,134,54,40);
    border: 1px solid rgba(35,134,54,120);
    border-radius: 6px; color: #3fb950; font-weight: 500;
    padding: 0 12px; min-height: 28px; max-height: 28px;
}
QPushButton#autostartEnableBtn:hover { background: rgba(35,134,54,80); border-color: #3fb950; }
QPushButton#autostartDisableBtn {
    background: rgba(218,54,51,30);
    border: 1px solid rgba(218,54,51,100);
    border-radius: 6px; color: #f85149; font-weight: 500;
    padding: 0 12px; min-height: 28px; max-height: 28px;
}
QPushButton#autostartDisableBtn:hover { background: rgba(218,54,51,70); border-color: #f85149; }
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
QLabel#bindPrefix { color: #6e7681; font-size: 10px; }
QLabel#bindHint {
    color: #58a6ff; font-size: 10px; font-family: monospace;
    background: rgba(88,166,255,8);
    border: 1px solid rgba(88,166,255,25);
    border-radius: 5px; padding: 3px 7px;
}
)"; // APP_STYLE

static QLabel *makeLabel(const QString &text, const QString &obj = {})
{
    auto *l = new QLabel(text);
    l->setFixedWidth(LABEL_W);
    l->setFixedHeight(ROW_H);
    l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    l->setStyleSheet("color:#8b949e;");
    if (!obj.isEmpty()) l->setObjectName(obj);
    return l;
}

static QWidget *makeRow(QLabel *lbl, QWidget *input, QPushButton *btn = nullptr)
{
    auto *row = new QWidget;
    auto *hl  = new QHBoxLayout(row);
    hl->setContentsMargins(0,0,0,0);
    hl->setSpacing(8);
    hl->addWidget(lbl);
    hl->addWidget(input, 1);
    if (btn) hl->addWidget(btn);
    row->setFixedHeight(ROW_H + 4);
    return row;
}

static QString orientStr(int t, const Strings &s)
{
    switch (t%4) {
        case 0: return s.orientLandscape;
        case 1: return s.orientPortrait90;
        case 2: return s.orientLandscape180;
        case 3: return s.orientPortrait270;
        default: return "?";
    }
}

static QString autostartFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + "/autostart/hyprwall.desktop";
}
static QString autostartContent()
{
    return QString(
        "[Desktop Entry]\nType=Application\nName=HyprWall\n"
        "Exec=hyprwall --service\nHidden=false\nNoDisplay=false\n"
        "X-GNOME-Autostart-enabled=true\nComment=HyprWall wallpaper service\n");
}

// Returns path to the slideshow script (installed alongside binary or in PATH)
static QString slideshowScriptPath()
{
    // 1. next to the executable
    QString beside = QCoreApplication::applicationDirPath() + "/hyprwall-slideshow.sh";
    if (QFile::exists(beside)) return beside;
    // 2. /usr/local/share/hyprwall/
    QString shared = "/usr/local/share/hyprwall/hyprwall-slideshow.sh";
    if (QFile::exists(shared)) return shared;
    // 3. in PATH
    return "hyprwall-slideshow.sh";
}

QString MainWindow::smartBrowseDir() const
{
    const QString home = QDir::homePath();
    for (const QString &d : {
            home+"/Pictures/wallpapers",
            home+"/Pictures/wallpaper",
            home+"/Pictures",
            home })
        if (QDir(d).exists()) return d;
    return home;
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
    }
    void setMonitors(const QList<MonitorInfo> &m)
        { m_monitors=m; m_selected=m.isEmpty()?QString():m.first().name; update(); }
    void setSelected(const QString &n)           { m_selected=n; update(); }
    void setNoMonitorsText(const QString &t)     { m_noMon=t; update(); }
    // mode: 0=image, 1=video, 2=slideshow
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
            p.drawText(rect(),Qt::AlignCenter,m_noMon);
            return;
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
            QRect r(rx,ry,rw,rh);
            bool sel=(m.name==m_selected);
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
            } else if (mode==2) {
                // slideshow photo-stack icon
                p.fillRect(r,QColor(13,17,40));
                int sw=rw*2/3,sh=rh*2/3;
                int sx=rx+(rw-sw)/2,sy=ry+(rh-sh)/2;
                for (int i=2;i>=0;i--) {
                    int off=i*3;
                    QRect pr(sx+off,sy-off,sw,sh);
                    p.setPen(QPen(QColor(0x58,0xa6,0xff,100+i*40),1));
                    p.setBrush(i==0?QColor(35,40,70):QColor(25,30,55));
                    p.drawRoundedRect(pr,3,3);
                }
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(0xf7,0xcc,0x4b,200));
                p.drawEllipse(sx+sw*2/3,sy+sh/6,sw/6,sw/6);
                p.setBrush(QColor(0x58,0xa6,0xff,180));
                QPolygon tri;
                tri<<QPoint(sx+sw/6,sy+sh*4/5)<<QPoint(sx+sw*5/8,sy+sh*2/5)<<QPoint(sx+sw,sy+sh*4/5);
                p.drawPolygon(tri);
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
        const int P=16;int aW=width()-2*P,aH=height()-2*P;
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
    QString m_selected,m_noMon{"No monitors"};
    QMap<QString,int>     m_modes;
    QMap<QString,QPixmap> m_pixmaps;
};
#include "MainWindow.moc"

// ============================================================
// Slideshow process management
// ============================================================
void MainWindow::startSlideshowScript(const QString &monitor, const QString &folder, int secs)
{
    stopSlideshowScript(monitor);
    QString script = slideshowScriptPath();
    QStringList args;
    args << folder << QString::number(secs);
    auto *proc = new QProcess(this);
    proc->setProgram(script);
    proc->setArguments({monitor, folder, QString::number(secs)});
    proc->start();
    if (!proc->waitForStarted(2000)) {
        qWarning() << "Failed to start slideshow script for" << monitor;
        delete proc;
        return;
    }
    m_slideshowProcs[monitor] = proc;
    qDebug() << "Slideshow script started for" << monitor << "pid" << proc->processId();
}

void MainWindow::stopSlideshowScript(const QString &monitor)
{
    if (m_slideshowProcs.contains(monitor)) {
        QProcess *proc = m_slideshowProcs.take(monitor);
        if (proc->state() != QProcess::NotRunning) {
            proc->terminate();
            proc->waitForFinished(1000);
            if (proc->state() != QProcess::NotRunning)
                proc->kill();
        }
        delete proc;
        qDebug() << "Slideshow script stopped for" << monitor;
    }
    // Also kill any orphan instances for this monitor
    QProcess::execute("pkill", {"-f", QString("hyprwall-slideshow.sh.*%1").arg(monitor)});
}

void MainWindow::stopAllSlideshowScripts()
{
    for (const QString &mon : m_slideshowProcs.keys())
        stopSlideshowScript(mon);
}

// ============================================================
// Compat stubs (timer no longer used)
// ============================================================
void MainWindow::startSlideshowTimer() {}
void MainWindow::stopSlideshowTimer()  {}
void MainWindow::applyNextSlide()      {}
void MainWindow::onSlideshowTick()     {}

// ============================================================
// MainWindow
// ============================================================
bool MainWindow::isAutostartEnabled() const { return QFile::exists(autostartFilePath()); }

void MainWindow::updateAutostartButton()
{
    if (!m_autostartBtn) return;
    if (isAutostartEnabled()) {
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
    QString path = autostartFilePath();
    if (isAutostartEnabled()) {
        QFile::remove(path);
    } else {
        QDir dir=QFileInfo(path).dir(); dir.mkpath(".");
        QFile f(path);
        if (f.open(QIODevice::WriteOnly|QIODevice::Text))
            QTextStream(&f)<<autostartContent();
    }
    updateAutostartButton();
}

void MainWindow::updateModeStack(int idx) { if(m_modeStack) m_modeStack->setCurrentIndex(idx); }

void MainWindow::onModeChanged(int)
{
    int idx=m_radioSlideshow->isChecked()?1:0;
    updateModeStack(idx);
    if (idx==0) stopSlideshowScript(m_currentMonitor);
    if (!m_currentMonitor.isEmpty()) {
        if (idx==1)
            m_monitorBar->setMonitorMode(m_currentMonitor,2);
        else {
            bool vid=WallpaperApplier::isVideoFile(m_fileEdit->text());
            m_monitorBar->setMonitorMode(m_currentMonitor,vid?1:0,vid?QString():m_fileEdit->text());
        }
    }
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setAttribute(Qt::WA_TranslucentBackground,true);
    setWindowFlags(windowFlags()|Qt::FramelessWindowHint);
    m_s=stringsEN();
    qApp->setStyleSheet(APP_STYLE);
    ConfigManager::instance().load();
    buildUi();
    loadMonitors();
}

MainWindow::~MainWindow()
{
    stopAllSlideshowScripts();
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(13,17,23,218));
    p.setPen(QPen(QColor(0x30,0x36,0x3d,180),1));
    p.drawRoundedRect(rect().adjusted(1,1,-1,-1),12,12);
    QLinearGradient g(0,0,width(),0);
    g.setColorAt(0.0,QColor(0x38,0x8b,0xfd,0));
    g.setColorAt(0.2,QColor(0x58,0xa6,0xff,180));
    g.setColorAt(0.8,QColor(0x38,0x8b,0xfd,180));
    g.setColorAt(1.0,QColor(0x38,0x8b,0xfd,0));
    p.setPen(Qt::NoPen); p.setBrush(g);
    p.drawRoundedRect(QRect(0,0,width(),2),1,1);
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    if(e->button()==Qt::LeftButton)
        m_dragPos=e->globalPosition().toPoint()-frameGeometry().topLeft();
}
void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    if(e->buttons()&Qt::LeftButton)
        move(e->globalPosition().toPoint()-m_dragPos);
}

void MainWindow::buildUi()
{
    setWindowTitle(m_s.windowTitle);
    setMinimumWidth(460);
    setMinimumHeight(600);
    resize(560,720);

    // Central widget with a scroll area so content never gets clipped on resize
    QWidget *central=new QWidget(this);
    central->setObjectName("central");
    setCentralWidget(central);

    QVBoxLayout *outerLayout=new QVBoxLayout(central);
    outerLayout->setContentsMargins(0,0,0,0);
    outerLayout->setSpacing(0);

    // All content goes into a single VBox inside a scroll area
    QScrollArea *scroll=new QScrollArea;
    scroll->setObjectName("scrollArea");
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget *contents=new QWidget;
    contents->setObjectName("scrollContents");
    QVBoxLayout *root=new QVBoxLayout(contents);
    root->setSpacing(8);
    root->setContentsMargins(16,12,16,12);
    // KEY: no trailing stretch — items pack to top, no gaps
    root->setAlignment(Qt::AlignTop);

    scroll->setWidget(contents);
    outerLayout->addWidget(scroll);

    // ─ Title bar
    {
        QHBoxLayout *tb=new QHBoxLayout; tb->setSpacing(6);
        QLabel *title=new QLabel("HyprWall");
        title->setStyleSheet("font-size:17px;font-weight:700;color:#c9d1d9;letter-spacing:1px;");
        m_autostartLabel=new QLabel(m_s.autostartLabel);
        m_autostartLabel->setStyleSheet("color:#8b949e;font-size:12px;");
        m_autostartBtn=new QPushButton; m_autostartBtn->setFixedWidth(90);
        updateAutostartButton();
        connect(m_autostartBtn,&QPushButton::clicked,this,&MainWindow::onAutostartToggle);
        m_langLabel=new QLabel(m_s.langLabel);
        m_langLabel->setStyleSheet("color:#8b949e;font-size:12px;");
        m_langCombo=new QComboBox;
        m_langCombo->addItems({"English","\u0420\u0443\u0441\u0441\u043a\u0438\u0439"});
        m_langCombo->setFixedWidth(110);
        connect(m_langCombo,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&MainWindow::onLanguageChanged);
        QPushButton *closeBtn=new QPushButton("\u2715");
        closeBtn->setFixedSize(26,26);
        closeBtn->setStyleSheet(
            "QPushButton{background:transparent;border:1px solid rgba(255,70,70,50);"
            "border-radius:5px;color:#6e7681;font-size:11px;padding:0;"
            "min-height:26px;max-height:26px;}"
            "QPushButton:hover{background:rgba(218,54,51,180);border-color:transparent;color:#fff;}");
        connect(closeBtn,&QPushButton::clicked,this,&QMainWindow::close);
        tb->addWidget(title); tb->addStretch();
        tb->addWidget(m_autostartLabel); tb->addWidget(m_autostartBtn);
        tb->addSpacing(10);
        tb->addWidget(m_langLabel); tb->addWidget(m_langCombo);
        tb->addSpacing(6); tb->addWidget(closeBtn);
        root->addLayout(tb);
    }

    // ─ Monitor bar
    m_monitorBar=new MonitorBar(this);
    m_monitorBar->setFixedHeight(160);
    m_monitorBar->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    m_monitorBar->setNoMonitorsText(m_s.noMonitors);
    connect(m_monitorBar,&MonitorBar::monitorClicked,this,[this](const QString &name){
        int idx=m_monitorCombo->findText(name);
        if(idx>=0) m_monitorCombo->setCurrentIndex(idx);
    });
    root->addWidget(m_monitorBar);

    // ─ Monitor selector
    {
        m_monitorLabel=makeLabel(m_s.monitorLabel,"monitorLabel");
        m_monitorCombo=new QComboBox;
        root->addWidget(makeRow(m_monitorLabel,m_monitorCombo));
    }

    // ─ Settings group
    m_settingsGroup=new QGroupBox(m_s.groupTitle);
    m_settingsGroup->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    QVBoxLayout *sg=new QVBoxLayout(m_settingsGroup);
    sg->setSpacing(8); sg->setContentsMargins(10,14,10,10);

    m_orientationLabel=new QLabel("-");
    m_orientationLabel->setObjectName("orientLabel");
    sg->addWidget(m_orientationLabel);

    // Mode bar
    {
        QWidget *modeBar=new QWidget; modeBar->setObjectName("modeBar");
        QHBoxLayout *ml=new QHBoxLayout(modeBar);
        ml->setContentsMargins(4,4,4,4); ml->setSpacing(4);
        m_radioStatic=new QRadioButton(m_s.modeStatic); m_radioStatic->setChecked(true);
        m_radioSlideshow=new QRadioButton(m_s.modeSlideshow);
        m_modeGroup=new QButtonGroup(this);
        m_modeGroup->addButton(m_radioStatic,0);
        m_modeGroup->addButton(m_radioSlideshow,1);
        connect(m_modeGroup,QOverload<int>::of(&QButtonGroup::idClicked),this,&MainWindow::onModeChanged);
        ml->addWidget(m_radioStatic); ml->addWidget(m_radioSlideshow); ml->addStretch();
        sg->addWidget(modeBar);
    }

    // Stacked pages
    m_modeStack=new QStackedWidget;
    m_modeStack->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Minimum);

    // Page 0: Static
    {
        QWidget *page=new QWidget;
        QVBoxLayout *vl=new QVBoxLayout(page);
        vl->setContentsMargins(0,0,0,0); vl->setSpacing(6); vl->setAlignment(Qt::AlignTop);

        m_fileLabel=makeLabel(m_s.fileLabel,"fileLabel");
        m_fileEdit=new QLineEdit;
        m_browseBtn=new QPushButton(m_s.browseBtn); m_browseBtn->setFixedWidth(BTN_W);
        connect(m_browseBtn,&QPushButton::clicked,this,&MainWindow::onBrowseFile);
        vl->addWidget(makeRow(m_fileLabel,m_fileEdit,m_browseBtn));

        // Audio checkbox
        m_audioCheck=new QCheckBox(m_s.audioCheck);
        connect(m_audioCheck,&QCheckBox::toggled,this,&MainWindow::onAudioToggled);
        QWidget *audioWidget=new QWidget;
        QHBoxLayout *ar=new QHBoxLayout(audioWidget);
        ar->setContentsMargins(LABEL_W+8,0,0,0); ar->setSpacing(0);
        ar->addWidget(m_audioCheck); ar->addStretch();
        audioWidget->hide();
        m_audioCheck->setProperty("rowWidget",QVariant::fromValue(audioWidget));
        vl->addWidget(audioWidget);

        // Volume
        QWidget *vw=new QWidget;
        QHBoxLayout *volRow=new QHBoxLayout(vw);
        volRow->setContentsMargins(0,0,0,0); volRow->setSpacing(8);
        m_volumeLabelW=makeLabel(m_s.volumeLabel,"volumeLabel");
        m_volumeSlider=new QSlider(Qt::Horizontal);
        m_volumeSlider->setRange(0,100); m_volumeSlider->setValue(50);
        m_volumeLabel=new QLabel("50%");
        m_volumeLabel->setFixedWidth(36);
        m_volumeLabel->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
        m_volumeLabel->setStyleSheet("color:#58a6ff;font-weight:600;");
        connect(m_volumeSlider,&QSlider::valueChanged,this,&MainWindow::onVolumeChanged);
        volRow->addWidget(m_volumeLabelW); volRow->addWidget(m_volumeSlider,1); volRow->addWidget(m_volumeLabel);
        vw->hide();
        m_volumeSlider->setProperty("volWidget",QVariant::fromValue(vw));
        vl->addWidget(vw);

        // Bind hint
        m_bindRow=new QWidget;
        QVBoxLayout *bl=new QVBoxLayout(m_bindRow);
        bl->setContentsMargins(LABEL_W+8,4,0,0); bl->setSpacing(3);
        m_bindPrefixLabel=new QLabel(m_s.bindPrefix); m_bindPrefixLabel->setObjectName("bindPrefix");
        m_bindHint=new QLabel; m_bindHint->setObjectName("bindHint");
        m_bindHint->setTextInteractionFlags(Qt::TextSelectableByMouse);
        bl->addWidget(m_bindPrefixLabel); bl->addWidget(m_bindHint);
        m_bindRow->hide();
        vl->addWidget(m_bindRow);
        m_modeStack->addWidget(page);
    }

    // Page 1: Slideshow
    {
        QWidget *page=new QWidget;
        QVBoxLayout *vl=new QVBoxLayout(page);
        vl->setContentsMargins(0,0,0,0); vl->setSpacing(6); vl->setAlignment(Qt::AlignTop);

        m_folderLabel=makeLabel(m_s.folderLabel,"folderLabel");
        m_folderEdit=new QLineEdit;
        m_browseFolderBtn=new QPushButton(m_s.browseFolderBtn); m_browseFolderBtn->setFixedWidth(BTN_W);
        connect(m_browseFolderBtn,&QPushButton::clicked,this,&MainWindow::onBrowseFolder);
        vl->addWidget(makeRow(m_folderLabel,m_folderEdit,m_browseFolderBtn));

        m_intervalLabel=makeLabel(m_s.intervalLabel,"intervalLabel");
        m_intervalCombo=new QComboBox;
        m_intervalCombo->addItems(m_s.intervalLabels);
        vl->addWidget(makeRow(m_intervalLabel,m_intervalCombo));
        m_modeStack->addWidget(page);
    }

    sg->addWidget(m_modeStack);

    // Fill
    {
        m_fillLabel=makeLabel(m_s.fillLabel,"fillLabel");
        m_fillCombo=new QComboBox;
        m_fillCombo->addItems(m_s.imgFillModes);
        sg->addWidget(makeRow(m_fillLabel,m_fillCombo));
    }
    // Rotation
    {
        m_rotLabel=makeLabel(m_s.rotLabel,"rotLabel");
        m_rotCombo=new QComboBox;
        m_rotCombo->addItems(m_s.imgRotModes);
        sg->addWidget(makeRow(m_rotLabel,m_rotCombo));
    }

    // Apply
    m_applyBtn=new QPushButton(m_s.applyBtn);
    m_applyBtn->setObjectName("applyBtn");
    connect(m_applyBtn,&QPushButton::clicked,this,&MainWindow::onApply);
    sg->addWidget(m_applyBtn);

    root->addWidget(m_settingsGroup);
    // NO addStretch() — AlignTop handles packing

    connect(m_monitorCombo,QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,&MainWindow::onMonitorSelected);
}

void MainWindow::switchToVideo(bool isVideo)
{
    if(m_isVideo==isVideo) return;
    m_isVideo=isVideo;
    int pf=m_fillCombo->currentIndex(),pr=m_rotCombo->currentIndex();
    m_fillCombo->blockSignals(true); m_rotCombo->blockSignals(true);
    m_fillCombo->clear(); m_rotCombo->clear();
    if(isVideo){m_fillCombo->addItems(m_s.vidFillModes);m_rotCombo->addItems(m_s.vidRotModes);}
    else       {m_fillCombo->addItems(m_s.imgFillModes);m_rotCombo->addItems(m_s.imgRotModes);}
    m_fillCombo->setCurrentIndex(std::min(pf,m_fillCombo->count()-1));
    m_rotCombo->setCurrentIndex(std::min(pr,m_rotCombo->count()-1));
    m_fillCombo->blockSignals(false); m_rotCombo->blockSignals(false);
}

void MainWindow::retranslateUi()
{
    setWindowTitle(m_s.windowTitle);
    m_langLabel->setText(m_s.langLabel);
    m_monitorBar->setNoMonitorsText(m_s.noMonitors);
    m_monitorLabel->setText(m_s.monitorLabel);
    m_settingsGroup->setTitle(m_s.groupTitle);
    m_fileLabel->setText(m_s.fileLabel);
    m_browseBtn->setText(m_s.browseBtn);
    m_audioCheck->setText(m_s.audioCheck);
    m_volumeLabelW->setText(m_s.volumeLabel);
    m_fillLabel->setText(m_s.fillLabel);
    m_rotLabel->setText(m_s.rotLabel);
    m_applyBtn->setText(m_s.applyBtn);
    m_bindPrefixLabel->setText(m_s.bindPrefix);
    m_autostartLabel->setText(m_s.autostartLabel);
    m_folderLabel->setText(m_s.folderLabel);
    m_intervalLabel->setText(m_s.intervalLabel);
    m_radioStatic->setText(m_s.modeStatic);
    m_radioSlideshow->setText(m_s.modeSlideshow);
    m_browseFolderBtn->setText(m_s.browseFolderBtn);
    int pi=m_intervalCombo->currentIndex();
    m_intervalCombo->blockSignals(true); m_intervalCombo->clear();
    m_intervalCombo->addItems(m_s.intervalLabels);
    m_intervalCombo->setCurrentIndex(qBound(0,pi,3));
    m_intervalCombo->blockSignals(false);
    updateAutostartButton();
    int fi=m_fillCombo->currentIndex(),ri=m_rotCombo->currentIndex();
    m_fillCombo->blockSignals(true); m_rotCombo->blockSignals(true);
    m_fillCombo->clear(); m_rotCombo->clear();
    if(m_isVideo){m_fillCombo->addItems(m_s.vidFillModes);m_rotCombo->addItems(m_s.vidRotModes);}
    else         {m_fillCombo->addItems(m_s.imgFillModes);m_rotCombo->addItems(m_s.imgRotModes);}
    m_fillCombo->setCurrentIndex(std::min(fi,m_fillCombo->count()-1));
    m_rotCombo->setCurrentIndex(std::min(ri,m_rotCombo->count()-1));
    m_fillCombo->blockSignals(false); m_rotCombo->blockSignals(false);
    if(!m_currentMonitor.isEmpty()) populateSettings(m_currentMonitor);
    m_monitorBar->update();
}

void MainWindow::onLanguageChanged(int idx)
{
    m_isRU=(idx==1);
    m_s=m_isRU?stringsRU():stringsEN();
    retranslateUi();
}

QString MainWindow::bindString() const
{
    return QString("bind = SUPER, F9, exec, hyprwall --toggle-audio %1").arg(m_currentMonitor);
}

void MainWindow::loadMonitors()
{
    m_monitors=MonitorDetector::detect();
    m_monitorBar->setMonitors(m_monitors);
    m_monitorCombo->blockSignals(true); m_monitorCombo->clear();
    auto &cm=ConfigManager::instance();
    for (const MonitorInfo &m:m_monitors) {
        m_monitorCombo->addItem(m.name);
        WallpaperConfig cfg=cm.getConfig(m.name);
        if (cfg.mode==WallpaperMode::Slideshow)
            m_monitorBar->setMonitorMode(m.name,2);
        else if (!cfg.filePath.isEmpty()) {
            bool vid=WallpaperApplier::isVideoFile(cfg.filePath);
            m_monitorBar->setMonitorMode(m.name,vid?1:0,vid?QString():cfg.filePath);
        }
    }
    m_monitorCombo->blockSignals(false);
    if(!m_monitors.isEmpty()){m_monitorCombo->setCurrentIndex(0);onMonitorSelected(0);}
}

void MainWindow::onMonitorSelected(int index)
{
    if(index<0||index>=m_monitors.size()) return;
    saveCurrentSettings();
    m_currentMonitor=m_monitors[index].name;
    m_monitorBar->setSelected(m_currentMonitor);
    populateSettings(m_currentMonitor);
}

void MainWindow::populateSettings(const QString &monitorName)
{
    auto it=std::find_if(m_monitors.cbegin(),m_monitors.cend(),
        [&](const MonitorInfo &m){return m.name==monitorName;});
    if(it!=m_monitors.cend())
        m_orientationLabel->setText(
            QString("%1  |  %2x%3  @  %4Hz  scale %5")
            .arg(orientStr(it->transform,m_s))
            .arg(it->width).arg(it->height).arg(it->refreshRate)
            .arg(it->scale,0,'f',2));

    WallpaperConfig cfg=ConfigManager::instance().getConfig(monitorName);
    bool isSlide=(cfg.mode==WallpaperMode::Slideshow);
    m_radioStatic->setChecked(!isSlide);
    m_radioSlideshow->setChecked(isSlide);
    updateModeStack(isSlide?1:0);

    m_folderEdit->setText(cfg.folderPath);
    int intIdx=0;
    for(int i=0;i<4;i++){if(INTERVAL_SECS[i]==cfg.slideshowSecs){intIdx=i;break;}}
    m_intervalCombo->setCurrentIndex(intIdx);

    bool isVid=WallpaperApplier::isVideoFile(cfg.filePath);
    m_isVideo=!isVid; switchToVideo(isVid);
    m_fillCombo->blockSignals(true); m_rotCombo->blockSignals(true);
    m_fileEdit->setText(cfg.filePath);
    m_fillCombo->setCurrentIndex(static_cast<int>(cfg.fillMode));
    m_rotCombo->setCurrentIndex(static_cast<int>(cfg.rotation));
    m_audioCheck->setChecked(cfg.audioEnabled);
    m_volumeSlider->setValue(cfg.audioVolume);
    m_volumeLabel->setText(QString("%1%").arg(cfg.audioVolume));
    m_fillCombo->blockSignals(false); m_rotCombo->blockSignals(false);

    bool showAudio=isVid&&!isSlide;
    QWidget *audioWidget=m_audioCheck->property("rowWidget").value<QWidget*>();
    if(audioWidget) audioWidget->setVisible(showAudio);
    m_audioCheck->setVisible(showAudio);
    QWidget *vw=m_volumeSlider->property("volWidget").value<QWidget*>();
    if(vw) vw->setVisible(showAudio&&cfg.audioEnabled);
    if(showAudio){m_bindHint->setText(bindString());m_bindRow->show();}
    else m_bindRow->hide();

    if(isSlide)
        m_monitorBar->setMonitorMode(monitorName,2);
    else
        m_monitorBar->setMonitorMode(monitorName,isVid?1:0,isVid?QString():cfg.filePath);
    // Don't auto-start timer on populate; only on Apply
}

void MainWindow::saveCurrentSettings()
{
    if(m_currentMonitor.isEmpty()) return;
    WallpaperConfig cfg;
    cfg.monitorName  =m_currentMonitor;
    cfg.filePath     =m_fileEdit->text();
    cfg.folderPath   =m_folderEdit->text();
    cfg.mode         =m_radioSlideshow->isChecked()?WallpaperMode::Slideshow:WallpaperMode::Static;
    cfg.slideshowSecs=INTERVAL_SECS[qBound(0,m_intervalCombo->currentIndex(),3)];
    cfg.fillMode     =static_cast<FillMode>(m_fillCombo->currentIndex());
    cfg.rotation     =static_cast<WallpaperRotation>(m_rotCombo->currentIndex());
    cfg.audioEnabled =m_audioCheck->isChecked();
    cfg.audioVolume  =m_volumeSlider->value();
    ConfigManager::instance().setConfig(m_currentMonitor,cfg);
}

void MainWindow::onBrowseFile()
{
    QString title =m_isRU?QString("\u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0444\u0430\u0439\u043b"):QString("Select file");
    QString filter=m_isRU?QString("\u0418\u0437\u043e\u0431\u0440\u0430\u0436\u0435\u043d\u0438\u044f \u0438 \u0432\u0438\u0434\u0435\u043e"):QString("Images and video");
    filter+=" (*.jpg *.jpeg *.png *.bmp *.gif *.mp4 *.mkv *.avi *.webm *.mov);;";
    filter+=m_isRU?QString("\u0412\u0441\u0435 \u0444\u0430\u0439\u043b\u044b (*)"):QString("All files (*)");
    QString path=QFileDialog::getOpenFileName(this,title,smartBrowseDir(),filter);
    if(path.isEmpty()) return;
    m_fileEdit->setText(path);
    bool isVid=WallpaperApplier::isVideoFile(path);
    switchToVideo(isVid);
    QWidget *audioWidget=m_audioCheck->property("rowWidget").value<QWidget*>();
    if(audioWidget) audioWidget->setVisible(isVid);
    m_audioCheck->setVisible(isVid);
    QWidget *vw=m_volumeSlider->property("volWidget").value<QWidget*>();
    if(vw) vw->setVisible(isVid&&m_audioCheck->isChecked());
    if(isVid){m_bindHint->setText(bindString());m_bindRow->show();}else m_bindRow->hide();
    m_monitorBar->setMonitorMode(m_currentMonitor,isVid?1:0,isVid?QString():path);
}

void MainWindow::onBrowseFolder()
{
    QString title=m_isRU?QString("\u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u043f\u0430\u043f\u043a\u0443"):QString("Select folder");
    QString path=QFileDialog::getExistingDirectory(this,title,smartBrowseDir());
    if(path.isEmpty()) return;
    m_folderEdit->setText(path);
    m_monitorBar->setMonitorMode(m_currentMonitor,2);
}

void MainWindow::onApply()
{
    saveCurrentSettings();
    ConfigManager::instance().save();
    WallpaperConfig cfg=ConfigManager::instance().getConfig(m_currentMonitor);

    if(cfg.mode==WallpaperMode::Slideshow) {
        if(cfg.folderPath.isEmpty()) {
            QMessageBox::warning(this,m_s.errTitle,
                m_isRU?QString("\u041f\u0430\u043f\u043a\u0430 \u043d\u0435 \u0432\u044b\u0431\u0440\u0430\u043d\u0430"):QString("No folder selected"));
            return;
        }
        int secs=INTERVAL_SECS[qBound(0,m_intervalCombo->currentIndex(),3)];
        startSlideshowScript(m_currentMonitor, cfg.folderPath, secs);
        m_monitorBar->setMonitorMode(m_currentMonitor,2);
    } else {
        stopSlideshowScript(m_currentMonitor);
        if(!WallpaperApplier::apply(cfg))
            QMessageBox::warning(this,m_s.errTitle,m_s.errBody);
    }
}

void MainWindow::onFillModeChanged(int) {}
void MainWindow::onRotationChanged(int) {}

void MainWindow::onAudioToggled(bool checked)
{
    QWidget *vw=m_volumeSlider->property("volWidget").value<QWidget*>();
    if(vw) vw->setVisible(checked);
}

void MainWindow::onVolumeChanged(int val)
{
    m_volumeLabel->setText(QString("%1%").arg(val));
}
