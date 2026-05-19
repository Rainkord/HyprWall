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
#include <algorithm>
#include <climits>

// GitHub Dark palette
static const char *APP_STYLE = R"(
* {
    font-family: 'Segoe UI', 'Inter', sans-serif;
    font-size: 13px;
    color: #c9d1d9;
}
QMainWindow, QWidget#central {
    background: transparent;
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
    background: rgba(13, 17, 23, 200);
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 5px 10px;
    color: #c9d1d9;
    selection-background-color: #388bfd;
    selection-color: #ffffff;
}
QLineEdit:focus {
    border: 1px solid #58a6ff;
    background: rgba(13, 17, 23, 230);
}
QComboBox {
    background: rgba(22, 27, 34, 220);
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 5px 10px;
    color: #c9d1d9;
    min-height: 26px;
}
QComboBox:hover {
    border: 1px solid #58a6ff;
}
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
    background: rgba(48, 54, 61, 220);
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 6px 16px;
    color: #c9d1d9;
    font-weight: 500;
}
QPushButton:hover {
    background: rgba(56, 139, 253, 30);
    border: 1px solid #388bfd;
    color: #58a6ff;
}
QPushButton:pressed {
    background: rgba(56, 139, 253, 20);
}
QPushButton#applyBtn {
    background: #238636;
    border: 1px solid rgba(240,246,252,0.1);
    border-radius: 8px;
    color: #ffffff;
    font-weight: 600;
    font-size: 14px;
    padding: 9px 0;
    letter-spacing: 0.5px;
}
QPushButton#applyBtn:hover {
    background: #2ea043;
    border-color: rgba(240,246,252,0.15);
}
QPushButton#applyBtn:pressed {
    background: #238636;
}
QPushButton#autostartEnableBtn {
    background: rgba(35,134,54,40);
    border: 1px solid rgba(35,134,54,120);
    border-radius: 6px;
    color: #3fb950;
    font-weight: 500;
    padding: 5px 12px;
}
QPushButton#autostartEnableBtn:hover {
    background: rgba(35,134,54,80);
    border-color: #3fb950;
}
QPushButton#autostartDisableBtn {
    background: rgba(218,54,51,30);
    border: 1px solid rgba(218,54,51,100);
    border-radius: 6px;
    color: #f85149;
    font-weight: 500;
    padding: 5px 12px;
}
QPushButton#autostartDisableBtn:hover {
    background: rgba(218,54,51,70);
    border-color: #f85149;
}
QCheckBox {
    spacing: 8px;
    color: #8b949e;
}
QCheckBox::indicator {
    width: 15px; height: 15px;
    border: 1px solid #30363d;
    border-radius: 3px;
    background: rgba(13,17,23,200);
}
QCheckBox::indicator:checked {
    background: #238636;
    border-color: #2ea043;
}
QSlider::groove:horizontal {
    height: 4px;
    background: #21262d;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background: #58a6ff;
    border: 2px solid #0d1117;
    width: 13px; height: 13px;
    margin: -5px 0;
    border-radius: 7px;
}
QSlider::sub-page:horizontal {
    background: #388bfd;
    border-radius: 2px;
}
QLabel#orientLabel {
    color: #8b949e;
    font-size: 11px;
}
QLabel#bindPrefix { color: #6e7681; font-size: 10px; }
QLabel#bindHint {
    color: #58a6ff;
    font-size: 10px;
    font-family: monospace;
    background: rgba(88,166,255,8);
    border: 1px solid rgba(88,166,255,25);
    border-radius: 5px;
    padding: 3px 7px;
}
QLabel#langLabel, QLabel#monitorLabel, QLabel#fileLabel,
QLabel#volumeLabel, QLabel#fillLabel, QLabel#rotLabel,
QLabel#autostartLabel {
    color: #8b949e;
    min-width: 72px;
}
)";  // end APP_STYLE

static QString orientStr(int transform, const Strings &s)
{
    switch (transform % 4) {
        case 0: return s.orientLandscape;
        case 1: return s.orientPortrait90;
        case 2: return s.orientLandscape180;
        case 3: return s.orientPortrait270;
        default: return "?";
    }
}

// ── autostart helpers ───────────────────────────────────────
static QString autostartFilePath()
{
    // ~/.config/autostart/hyprwall.desktop
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + "/autostart/hyprwall.desktop";
}

static QString autostartContent()
{
    return QString(
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=HyprWall\n"
        "Exec=hyprwall --service\n"
        "Hidden=false\n"
        "NoDisplay=false\n"
        "X-GNOME-Autostart-enabled=true\n"
        "Comment=HyprWall wallpaper service\n"
    );
}

// ============================================================
// MonitorBar
// ============================================================
class MonitorBar : public QWidget {
    Q_OBJECT
public:
    explicit MonitorBar(QWidget *p = nullptr) : QWidget(p) {
        setMinimumHeight(170);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    void setMonitors(const QList<MonitorInfo> &m)  { m_monitors=m; m_selected=m.isEmpty()?"":m.first().name; update(); }
    void setSelected(const QString &n)              { m_selected=n; update(); }
    void setNoMonitorsText(const QString &t)        { m_noMon=t; update(); }
    void setWallpaperPath(const QString &mon, const QString &path) {
        if (path.isEmpty()) return;
        if (!WallpaperApplier::isVideoFile(path)) {
            QPixmap px(path);
            if (!px.isNull()) m_pixmaps[mon] = px;
        } else {
            m_pixmaps.remove(mon);
        }
        m_wallpapers[mon] = path;
        update();
    }
signals:
    void monitorClicked(const QString &name);
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        p.setPen(QPen(QColor(0x30, 0x36, 0x3d, 200), 1));
        p.setBrush(QColor(13, 17, 23, 210));
        p.drawRoundedRect(rect().adjusted(0,0,-1,-1), 10, 10);

        if (m_monitors.isEmpty()) {
            p.setPen(QColor(0x8b, 0x94, 0x9e));
            p.drawText(rect(), Qt::AlignCenter, m_noMon);
            return;
        }
        int mnX=INT_MAX,mnY=INT_MAX,mxX=INT_MIN,mxY=INT_MIN;
        for (auto &m:m_monitors){mnX=std::min(mnX,m.x);mnY=std::min(mnY,m.y);mxX=std::max(mxX,m.x+m.width);mxY=std::max(mxY,m.y+m.height);}
        int tW=mxX-mnX, tH=mxY-mnY;
        if (!tW || !tH) return;
        const int P = 16;
        int aW=width()-2*P, aH=height()-2*P;
        double sc = std::min((double)aW/tW, (double)aH/tH);
        int oX=P+(aW-(int)(tW*sc))/2, oY=P+(aH-(int)(tH*sc))/2;

        for (auto &m : m_monitors) {
            int rx=oX+(int)((m.x-mnX)*sc);
            int ry=oY+(int)((m.y-mnY)*sc);
            int rw=std::max(6,(int)(m.width*sc));
            int rh=std::max(6,(int)(m.height*sc));
            QRect r(rx, ry, rw, rh);
            bool sel = (m.name == m_selected);

            if (m_pixmaps.contains(m.name)) {
                // Cover: scale keeping aspect ratio, then centre-crop
                const QPixmap &px = m_pixmaps[m.name];
                QSize scaled = px.size().scaled(r.size(), Qt::KeepAspectRatioByExpanding);
                QPixmap scaledPx = px.scaled(scaled, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                int cx = (scaledPx.width()  - r.width())  / 2;
                int cy = (scaledPx.height() - r.height()) / 2;
                p.setClipRect(r);
                p.drawPixmap(r.topLeft(), scaledPx, QRect(cx, cy, r.width(), r.height()));
                p.setClipping(false);
            } else if (m_wallpapers.contains(m.name) && WallpaperApplier::isVideoFile(m_wallpapers[m.name])) {
                // Video: dark bg + play icon, NO stretching
                p.fillRect(r, QColor(16, 10, 30));
                p.setPen(QColor(139, 92, 246));
                QFont f = p.font(); f.setPointSize(15); p.setFont(f);
                p.drawText(r, Qt::AlignCenter, "\u25b6");
                QFont rf = p.font(); rf.setPointSize(7); p.setFont(rf);
            } else {
                p.fillRect(r, QColor(22, 27, 34));
            }

            if (sel) {
                p.setPen(QPen(QColor(0x58, 0xa6, 0xff, 220), 2));
                p.setBrush(Qt::NoBrush);
                p.drawRect(r);
            } else {
                p.setPen(QPen(QColor(0x30, 0x36, 0x3d, 180), 1));
                p.setBrush(Qt::NoBrush);
                p.drawRect(r);
            }

            int lH = std::min(20, rh);
            QRect lr(rx, ry+rh-lH, rw, lH);
            p.fillRect(lr, QColor(0, 0, 0, 160));
            p.setPen(sel ? QColor(0x58, 0xa6, 0xff) : QColor(0xc9, 0xd1, 0xd9));
            QFont f = p.font(); f.setPointSize(7); f.setBold(sel); p.setFont(f);
            p.drawText(lr, Qt::AlignCenter, m.name);
        }
    }
    void mousePressEvent(QMouseEvent *ev) override {
        if (m_monitors.isEmpty()) return;
        int mnX=INT_MAX,mnY=INT_MAX,mxX=INT_MIN,mxY=INT_MIN;
        for (auto &m:m_monitors){mnX=std::min(mnX,m.x);mnY=std::min(mnY,m.y);mxX=std::max(mxX,m.x+m.width);mxY=std::max(mxY,m.y+m.height);}
        int tW=mxX-mnX, tH=mxY-mnY; if (!tW||!tH) return;
        const int P=16; int aW=width()-2*P, aH=height()-2*P;
        double sc=std::min((double)aW/tW,(double)aH/tH);
        int oX=P+(aW-(int)(tW*sc))/2, oY=P+(aH-(int)(tH*sc))/2;
        for (auto &m:m_monitors){
            int rx=oX+(int)((m.x-mnX)*sc), ry=oY+(int)((m.y-mnY)*sc);
            int rw=std::max(6,(int)(m.width*sc)), rh=std::max(6,(int)(m.height*sc));
            if (QRect(rx,ry,rw,rh).contains(ev->pos())) { emit monitorClicked(m.name); return; }
        }
    }
private:
    QList<MonitorInfo> m_monitors;
    QString m_selected, m_noMon{"No monitors"};
    QMap<QString,QString> m_wallpapers;
    QMap<QString,QPixmap> m_pixmaps;
};
#include "MainWindow.moc"

// ============================================================
// MainWindow
// ============================================================
bool MainWindow::isAutostartEnabled() const
{
    return QFile::exists(autostartFilePath());
}

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
    // re-apply style for objectName change
    m_autostartBtn->style()->unpolish(m_autostartBtn);
    m_autostartBtn->style()->polish(m_autostartBtn);
}

void MainWindow::onAutostartToggle()
{
    QString path = autostartFilePath();
    if (isAutostartEnabled()) {
        QFile::remove(path);
    } else {
        // ensure ~/.config/autostart/ exists
        QDir dir = QFileInfo(path).dir();
        dir.mkpath(".");
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream s(&f);
            s << autostartContent();
        }
    }
    updateAutostartButton();
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    m_s = stringsEN();
    qApp->setStyleSheet(APP_STYLE);
    ConfigManager::instance().load();
    buildUi();
    loadMonitors();
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(13, 17, 23, 218));
    p.setPen(QPen(QColor(0x30, 0x36, 0x3d, 180), 1));
    p.drawRoundedRect(rect().adjusted(1,1,-1,-1), 12, 12);
    QLinearGradient g(0, 0, width(), 0);
    g.setColorAt(0.0, QColor(0x38, 0x8b, 0xfd, 0));
    g.setColorAt(0.2, QColor(0x58, 0xa6, 0xff, 180));
    g.setColorAt(0.8, QColor(0x38, 0x8b, 0xfd, 180));
    g.setColorAt(1.0, QColor(0x38, 0x8b, 0xfd, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(g);
    p.drawRoundedRect(QRect(0, 0, width(), 2), 1, 1);
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

void MainWindow::buildUi()
{
    setWindowTitle(m_s.windowTitle);
    setMinimumSize(540, 660);
    resize(560, 700);

    QWidget *central = new QWidget(this);
    central->setObjectName("central");
    setCentralWidget(central);
    QVBoxLayout *root = new QVBoxLayout(central);
    root->setSpacing(10);
    root->setContentsMargins(16, 14, 16, 14);

    // Title bar
    {
        QHBoxLayout *tb = new QHBoxLayout;
        QLabel *title = new QLabel("HyprWall");
        title->setStyleSheet("font-size:17px;font-weight:700;color:#c9d1d9;letter-spacing:1px;");

        // Language
        m_langLabel = new QLabel(m_s.langLabel);
        m_langLabel->setObjectName("langLabel");
        m_langCombo = new QComboBox;
        m_langCombo->addItems({"English", "\u0420\u0443\u0441\u0441\u043a\u0438\u0439"});
        m_langCombo->setFixedWidth(110);
        connect(m_langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::onLanguageChanged);

        // Autostart
        m_autostartLabel = new QLabel(m_s.autostartLabel);
        m_autostartLabel->setObjectName("autostartLabel");
        m_autostartBtn = new QPushButton;
        m_autostartBtn->setFixedWidth(90);
        updateAutostartButton();
        connect(m_autostartBtn, &QPushButton::clicked, this, &MainWindow::onAutostartToggle);

        // Close button
        QPushButton *closeBtn = new QPushButton("\u2715");
        closeBtn->setFixedSize(26, 26);
        closeBtn->setStyleSheet(
            "QPushButton{background:transparent;border:1px solid rgba(255,70,70,50);"
            "border-radius:5px;color:#6e7681;font-size:11px;padding:0;}"
            "QPushButton:hover{background:rgba(218,54,51,180);border-color:transparent;color:#fff;}");
        connect(closeBtn, &QPushButton::clicked, this, &QMainWindow::close);

        tb->addWidget(title);
        tb->addStretch();
        tb->addWidget(m_autostartLabel);
        tb->addWidget(m_autostartBtn);
        tb->addSpacing(12);
        tb->addWidget(m_langLabel);
        tb->addWidget(m_langCombo);
        tb->addSpacing(8);
        tb->addWidget(closeBtn);
        root->addLayout(tb);
    }

    // Monitor bar
    m_monitorBar = new MonitorBar(this);
    m_monitorBar->setNoMonitorsText(m_s.noMonitors);
    connect(m_monitorBar, &MonitorBar::monitorClicked, this, [this](const QString &name){
        int idx = m_monitorCombo->findText(name);
        if (idx >= 0) m_monitorCombo->setCurrentIndex(idx);
    });
    root->addWidget(m_monitorBar);

    // Monitor selector
    {
        QHBoxLayout *row = new QHBoxLayout;
        m_monitorLabel = new QLabel(m_s.monitorLabel);
        m_monitorLabel->setObjectName("monitorLabel");
        m_monitorCombo = new QComboBox;
        row->addWidget(m_monitorLabel);
        row->addWidget(m_monitorCombo, 1);
        root->addLayout(row);
    }

    // Settings group
    m_settingsGroup = new QGroupBox(m_s.groupTitle);
    QVBoxLayout *sg = new QVBoxLayout(m_settingsGroup);
    sg->setSpacing(8);
    sg->setContentsMargins(10, 14, 10, 10);

    m_orientationLabel = new QLabel("-");
    m_orientationLabel->setObjectName("orientLabel");
    sg->addWidget(m_orientationLabel);

    // File row
    {
        QHBoxLayout *row = new QHBoxLayout;
        m_fileLabel = new QLabel(m_s.fileLabel);
        m_fileLabel->setObjectName("fileLabel");
        m_fileEdit = new QLineEdit;
        m_browseBtn = new QPushButton(m_s.browseBtn);
        m_browseBtn->setFixedWidth(80);
        connect(m_browseBtn, &QPushButton::clicked, this, &MainWindow::onBrowseFile);
        row->addWidget(m_fileLabel);
        row->addWidget(m_fileEdit, 1);
        row->addWidget(m_browseBtn);
        sg->addLayout(row);
    }

    // Audio
    m_audioCheck = new QCheckBox(m_s.audioCheck);
    connect(m_audioCheck, &QCheckBox::toggled, this, &MainWindow::onAudioToggled);
    m_audioCheck->hide();
    sg->addWidget(m_audioCheck);

    // Volume
    {
        QWidget *vw = new QWidget;
        QHBoxLayout *row = new QHBoxLayout(vw);
        row->setContentsMargins(0,0,0,0);
        m_volumeLabelW = new QLabel(m_s.volumeLabel);
        m_volumeLabelW->setObjectName("volumeLabel");
        m_volumeSlider = new QSlider(Qt::Horizontal);
        m_volumeSlider->setRange(0,100); m_volumeSlider->setValue(50);
        m_volumeLabel = new QLabel("50%");
        m_volumeLabel->setMinimumWidth(36);
        m_volumeLabel->setStyleSheet("color:#58a6ff;font-weight:600;");
        connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
        row->addWidget(m_volumeLabelW);
        row->addWidget(m_volumeSlider, 1);
        row->addWidget(m_volumeLabel);
        vw->hide();
        m_volumeSlider->setProperty("volWidget", QVariant::fromValue(vw));
        sg->addWidget(vw);
    }

    // Fill
    {
        QHBoxLayout *row = new QHBoxLayout;
        m_fillLabel = new QLabel(m_s.fillLabel);
        m_fillLabel->setObjectName("fillLabel");
        m_fillCombo = new QComboBox;
        m_fillCombo->addItems(m_s.imgFillModes);
        row->addWidget(m_fillLabel);
        row->addWidget(m_fillCombo, 1);
        sg->addLayout(row);
    }

    // Rotation
    {
        QHBoxLayout *row = new QHBoxLayout;
        m_rotLabel = new QLabel(m_s.rotLabel);
        m_rotLabel->setObjectName("rotLabel");
        m_rotCombo = new QComboBox;
        m_rotCombo->addItems(m_s.imgRotModes);
        row->addWidget(m_rotLabel);
        row->addWidget(m_rotCombo, 1);
        sg->addLayout(row);
    }

    // Bind hint
    {
        m_bindRow = new QWidget;
        QVBoxLayout *vl = new QVBoxLayout(m_bindRow);
        vl->setContentsMargins(0,4,0,0); vl->setSpacing(3);
        m_bindPrefixLabel = new QLabel(m_s.bindPrefix);
        m_bindPrefixLabel->setObjectName("bindPrefix");
        m_bindHint = new QLabel;
        m_bindHint->setObjectName("bindHint");
        m_bindHint->setTextInteractionFlags(Qt::TextSelectableByMouse);
        vl->addWidget(m_bindPrefixLabel);
        vl->addWidget(m_bindHint);
        m_bindRow->hide();
        sg->addWidget(m_bindRow);
    }

    // Apply
    m_applyBtn = new QPushButton(m_s.applyBtn);
    m_applyBtn->setObjectName("applyBtn");
    m_applyBtn->setFixedHeight(40);
    connect(m_applyBtn, &QPushButton::clicked, this, &MainWindow::onApply);
    sg->addWidget(m_applyBtn);

    root->addWidget(m_settingsGroup);
    root->addStretch();

    connect(m_monitorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onMonitorSelected);
}

void MainWindow::switchToVideo(bool isVideo)
{
    if (m_isVideo == isVideo) return;
    m_isVideo = isVideo;
    int prevFill = m_fillCombo->currentIndex();
    int prevRot  = m_rotCombo->currentIndex();
    m_fillCombo->blockSignals(true); m_rotCombo->blockSignals(true);
    m_fillCombo->clear(); m_rotCombo->clear();
    if (isVideo) {
        m_fillCombo->addItems(m_s.vidFillModes);
        m_rotCombo->addItems(m_s.vidRotModes);
    } else {
        m_fillCombo->addItems(m_s.imgFillModes);
        m_rotCombo->addItems(m_s.imgRotModes);
    }
    m_fillCombo->setCurrentIndex(std::min(prevFill, m_fillCombo->count()-1));
    m_rotCombo->setCurrentIndex(std::min(prevRot,  m_rotCombo->count()-1));
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
    updateAutostartButton();
    int fi=m_fillCombo->currentIndex(), ri=m_rotCombo->currentIndex();
    m_fillCombo->blockSignals(true); m_rotCombo->blockSignals(true);
    m_fillCombo->clear(); m_rotCombo->clear();
    if (m_isVideo) {
        m_fillCombo->addItems(m_s.vidFillModes);
        m_rotCombo->addItems(m_s.vidRotModes);
    } else {
        m_fillCombo->addItems(m_s.imgFillModes);
        m_rotCombo->addItems(m_s.imgRotModes);
    }
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

void MainWindow::loadMonitors()
{
    m_monitors = MonitorDetector::detect();
    m_monitorBar->setMonitors(m_monitors);
    m_monitorCombo->blockSignals(true);
    m_monitorCombo->clear();
    auto &cm = ConfigManager::instance();
    for (const MonitorInfo &m : m_monitors) {
        m_monitorCombo->addItem(m.name);
        WallpaperConfig cfg = cm.getConfig(m.name);
        if (!cfg.filePath.isEmpty())
            m_monitorBar->setWallpaperPath(m.name, cfg.filePath);
    }
    m_monitorCombo->blockSignals(false);
    if (!m_monitors.isEmpty()) { m_monitorCombo->setCurrentIndex(0); onMonitorSelected(0); }
}

void MainWindow::onMonitorSelected(int index)
{
    if (index < 0 || index >= m_monitors.size()) return;
    saveCurrentSettings();
    m_currentMonitor = m_monitors[index].name;
    m_monitorBar->setSelected(m_currentMonitor);
    populateSettings(m_currentMonitor);
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
    WallpaperConfig cfg = ConfigManager::instance().getConfig(monitorName);
    bool isVid = WallpaperApplier::isVideoFile(cfg.filePath);
    m_isVideo = !isVid;
    switchToVideo(isVid);
    m_fillCombo->blockSignals(true); m_rotCombo->blockSignals(true);
    m_fileEdit->setText(cfg.filePath);
    m_fillCombo->setCurrentIndex(static_cast<int>(cfg.fillMode));
    m_rotCombo->setCurrentIndex(static_cast<int>(cfg.rotation));
    m_audioCheck->setChecked(cfg.audioEnabled);
    m_volumeSlider->setValue(cfg.audioVolume);
    m_volumeLabel->setText(QString("%1%").arg(cfg.audioVolume));
    m_fillCombo->blockSignals(false); m_rotCombo->blockSignals(false);
    m_audioCheck->setVisible(isVid);
    QWidget *vw = m_volumeSlider->property("volWidget").value<QWidget*>();
    if (vw) vw->setVisible(isVid && cfg.audioEnabled);
    if (isVid) { m_bindHint->setText(bindString()); m_bindRow->show(); }
    else m_bindRow->hide();
}

void MainWindow::saveCurrentSettings()
{
    if (m_currentMonitor.isEmpty()) return;
    WallpaperConfig cfg;
    cfg.monitorName  = m_currentMonitor;
    cfg.filePath     = m_fileEdit->text();
    cfg.fillMode     = static_cast<FillMode>(m_fillCombo->currentIndex());
    cfg.rotation     = static_cast<WallpaperRotation>(m_rotCombo->currentIndex());
    cfg.audioEnabled = m_audioCheck->isChecked();
    cfg.audioVolume  = m_volumeSlider->value();
    ConfigManager::instance().setConfig(m_currentMonitor, cfg);
}

void MainWindow::onBrowseFile()
{
    QString title  = m_isRU ? QString("\u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0444\u0430\u0439\u043b") : QString("Select file");
    QString filter = m_isRU ? QString("\u0418\u0437\u043e\u0431\u0440\u0430\u0436\u0435\u043d\u0438\u044f \u0438 \u0432\u0438\u0434\u0435\u043e") : QString("Images and video");
    filter += " (*.jpg *.jpeg *.png *.bmp *.gif *.mp4 *.mkv *.avi *.webm *.mov);;";
    filter += m_isRU ? QString("\u0412\u0441\u0435 \u0444\u0430\u0439\u043b\u044b (*)") : QString("All files (*)");
    QString path = QFileDialog::getOpenFileName(this, title, QDir::homePath(), filter);
    if (path.isEmpty()) return;
    m_fileEdit->setText(path);
    bool isVid = WallpaperApplier::isVideoFile(path);
    switchToVideo(isVid);
    m_audioCheck->setVisible(isVid);
    QWidget *vw = m_volumeSlider->property("volWidget").value<QWidget*>();
    if (vw) vw->setVisible(isVid && m_audioCheck->isChecked());
    if (isVid) { m_bindHint->setText(bindString()); m_bindRow->show(); }
    else m_bindRow->hide();
    m_monitorBar->setWallpaperPath(m_currentMonitor, path);
}

void MainWindow::onApply()
{
    saveCurrentSettings();
    ConfigManager::instance().save();
    WallpaperConfig cfg = ConfigManager::instance().getConfig(m_currentMonitor);
    if (!WallpaperApplier::apply(cfg))
        QMessageBox::warning(this, m_s.errTitle, m_s.errBody);
}

void MainWindow::onFillModeChanged(int) {}
void MainWindow::onRotationChanged(int) {}

void MainWindow::onAudioToggled(bool checked)
{
    QWidget *vw = m_volumeSlider->property("volWidget").value<QWidget*>();
    if (vw) vw->setVisible(checked);
}

void MainWindow::onVolumeChanged(int val)
{
    m_volumeLabel->setText(QString("%1%").arg(val));
}
