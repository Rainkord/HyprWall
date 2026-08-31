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
#include "ToggleSwitch.h"
#include "MonitorBar.h"
#include "GalleryDelegate.h"
#include "GalleryConstants.h"
#include "ThumbCache.h"
#include "ImageCache.h"
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QFileSystemWatcher>
#include <algorithm>
#include <climits>
#include <cmath>

const int MainWindow::INTERVAL_VALUES[] = { 60, 300, 600, 900, 1800, 3600 };


// ============================================================
// Stylesheet — loaded from hyprwall.qss at runtime
// ============================================================
static QString loadStyleSheet()
{
    // Try: 1) next to binary, 2) ~/.config/hyprwall, 3) /usr/share/hyprwall
    const QStringList dirs = {
        QCoreApplication::applicationDirPath(),
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/hyprwall",
        QStringLiteral("/usr/share/hyprwall"),
        QStringLiteral("/usr/local/share/hyprwall")
    };
    for (const QString &base : dirs) {
        QFile f(base + "/hyprwall.qss");
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
            return QString::fromUtf8(f.readAll());
    }
    // Fallback: embedded minimal style
    return QStringLiteral(R"(
        * { font-family: 'Inter','Segoe UI',sans-serif; font-size:13px; color:#c9d1d9; }
        QMainWindow,QWidget#central,QScrollArea,QWidget#scrollContents { background:transparent; border:none; }
    )");
}

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
// MainWindow
// ============================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    qApp->setStyleSheet(loadStyleSheet());
    ConfigManager::instance().load();

    // Restore saved language from dedicated file
    int savedLang = ConfigManager::instance().loadLanguage();
    m_isRU = (savedLang == 1);
    m_s = m_isRU ? stringsRU() : stringsEN();

    buildUi();

    loadMonitors();
    startEntranceAnimation();
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

// ============================================================
// Entrance animation — fade-in + pulsing accent line
// ============================================================
void MainWindow::startEntranceAnimation()
{
    // Opacity entrance animation
    setWindowOpacity(0.0);
    m_entranceAnim = new QPropertyAnimation(this, "windowOpacity", this);
    m_entranceAnim->setDuration(400);
    m_entranceAnim->setStartValue(0.0);
    m_entranceAnim->setEndValue(1.0);
    m_entranceAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_entranceAnim->start(QAbstractAnimation::DeleteWhenStopped);
    connect(m_entranceAnim, &QPropertyAnimation::finished, this, [this]{ m_entranceDone = true; });
}

void MainWindow::animateSectionShow(QWidget *w)
{
    if (!w) return;
    w->show();
    w->setMaximumHeight(0);
    w->setMinimumHeight(0);
    QPropertyAnimation *anim = new QPropertyAnimation(w, "maximumHeight", this);
    anim->setDuration(250);
    anim->setStartValue(0);
    anim->setEndValue(w->sizeHint().height() + 20);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim, &QPropertyAnimation::finished, this, [w, anim]{
        w->setMaximumHeight(16777215);
        anim->deleteLater();
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::animateSectionHide(QWidget *w)
{
    if (!w || w->isHidden()) return;
    QPropertyAnimation *anim = new QPropertyAnimation(w, "maximumHeight", this);
    anim->setDuration(200);
    anim->setStartValue(w->height());
    anim->setEndValue(0);
    anim->setEasingCurve(QEasingCurve::InCubic);
    connect(anim, &QPropertyAnimation::finished, this, [w, anim]{
        w->hide();
        w->setMaximumHeight(16777215);
        anim->deleteLater();
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::showEvent(QShowEvent *ev)
{
    QMainWindow::showEvent(ev);
    if (!m_entranceDone) startEntranceAnimation();
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Window body — frosted glass
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(13, 17, 23, 220));
    p.drawRoundedRect(rect().adjusted(1,1,-1,-1), 14, 14);

    // Subtle inner border glow
    QLinearGradient borderGrad(0, 0, 0, height());
    borderGrad.setColorAt(0.0, QColor(56, 139, 253, 25));
    borderGrad.setColorAt(0.3, QColor(48, 54, 61, 15));
    borderGrad.setColorAt(1.0, QColor(48, 54, 61, 8));
    p.setPen(QPen(QColor(48, 54, 61, 40), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect().adjusted(1,1,-1,-1), 14, 14);
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
    setMinimumWidth(480);
    resize(580, 680);

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
    root->setSpacing(12);
    root->setContentsMargins(16,14,16,16);


    scroll->setWidget(contents);
    outerLayout->addWidget(scroll);

    // ── Title bar ────────────────────────────────────────────
    // Row 1: Title + close button
    {
        QHBoxLayout *tb = new QHBoxLayout;
        tb->setSpacing(0);
        tb->setContentsMargins(0, 0, 0, 0);
        QLabel *title = new QLabel("HyprWall");
        title->setStyleSheet("font-size:18px;font-weight:700;color:#e6edf3;letter-spacing:0.5px;");
        tb->addWidget(title); tb->addStretch();

        QPushButton *infoBtn = new QPushButton("\u2139");
        infoBtn->setFixedSize(28,28);
        infoBtn->setToolTip(m_isRU ? "О приложении" : "About");
        infoBtn->setStyleSheet(
            "QPushButton{background:transparent;border:none;"
            "border-radius:14px;color:#6e7681;font-size:14px;"
            "min-height:28px;max-height:28px;}"
            "QPushButton:hover{background:rgba(88,166,255,60);color:#58a6ff;}");
        connect(infoBtn, &QPushButton::clicked, this, &MainWindow::showAboutDialog);
        tb->addWidget(infoBtn);
        tb->addSpacing(4);

        QPushButton *closeBtn = new QPushButton("\u2715");
        closeBtn->setFixedSize(28,28);
        closeBtn->setStyleSheet(
            "QPushButton{background:transparent;border:none;"
            "border-radius:14px;color:#6e7681;font-size:13px;"
            "min-height:28px;max-height:28px;}"
            "QPushButton:hover{background:rgba(248,81,73,180);color:#fff;}");
        connect(closeBtn, &QPushButton::clicked, this, &QMainWindow::close);
        tb->addWidget(closeBtn);
        root->addLayout(tb);
    }
    // Row 2: Autostart + Language (secondary controls)
    {
        QHBoxLayout *row = new QHBoxLayout;
        row->setContentsMargins(0, 2, 0, 8);
        row->setSpacing(0);
        m_autostartLabel = new QLabel(m_s.autostartLabel);
        m_autostartLabel->setStyleSheet("color:#8b949e;font-size:12px;");
        m_autostartSwitch = new ToggleSwitch(this);
        m_autostartSwitch->setChecked(autostartEnabled(), false);
        connect(m_autostartSwitch, &ToggleSwitch::toggled,
                this, &MainWindow::onAutostartToggle);
        row->addWidget(m_autostartLabel);
        row->addSpacing(6);
        row->addWidget(m_autostartSwitch);
        row->addSpacing(20);
        m_langLabel = new QLabel(m_s.langLabel);
        m_langLabel->setStyleSheet("color:#8b949e;font-size:12px;");
        m_langCombo = new QComboBox;
        m_langCombo->addItems({"English", "\u0420\u0443\u0441\u0441\u043a\u0438\u0439"});
        m_langCombo->setFixedWidth(120);
        // Restore saved language index
        m_langCombo->blockSignals(true);
        m_langCombo->setCurrentIndex(ConfigManager::instance().loadLanguage());
        m_langCombo->blockSignals(false);
        connect(m_langCombo, &QComboBox::currentIndexChanged,
                this, &MainWindow::onLanguageChanged);
        row->addWidget(m_langLabel);
        row->addSpacing(6);
        row->addWidget(m_langCombo);
        row->addSpacing(20);
        // Same wallpaper toggle
        m_sameWallpaperLabel = new QLabel(m_s.sameWallpaperLabel);
        m_sameWallpaperLabel->setStyleSheet("color:#8b949e;font-size:12px;");
        m_sameWallpaperSwitch = new ToggleSwitch(this);
        m_sameWallpaperSwitch->setChecked(false, false);
        m_sameWallpaper = ConfigManager::instance().loadSameWallpaper();
        m_sameWallpaperSwitch->setChecked(m_sameWallpaper, false);
        connect(m_sameWallpaperSwitch, &ToggleSwitch::toggled,
                this, &MainWindow::onSameWallpaperToggled);
        row->addWidget(m_sameWallpaperLabel);
        row->addSpacing(6);
        row->addWidget(m_sameWallpaperSwitch);
        row->addStretch();
        root->addLayout(row);
        // Thin separator line
        QFrame *sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("background:#21262d;max-height:1px;");
        root->addWidget(sep);
    }

    // ── Tab bar (browser-style) ─────────────────────────────
    m_tabBar = new QWidget;
    m_tabBar->setFixedHeight(36);
    m_tabBar->setStyleSheet("background:transparent;");
    QHBoxLayout *tabLayout = new QHBoxLayout(m_tabBar);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->setSpacing(0);

    auto makeTabBtn = [this](const QString &text, int tabIdx) -> QPushButton* {
        QPushButton *btn = new QPushButton(text);
        btn->setCheckable(false);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumWidth(80);
        btn->setFixedHeight(34);
        btn->setProperty("tabIndex", tabIdx);
        connect(btn, &QPushButton::clicked, this, [this, tabIdx]{ switchTab(tabIdx); });
        return btn;
    };

    m_tabDesktopBtn = makeTabBtn(m_s.tabDesktop, 0);
    m_tabLockBtn    = makeTabBtn(m_s.tabLockScreen, 1);
    tabLayout->addWidget(m_tabDesktopBtn);
    tabLayout->addWidget(m_tabLockBtn);
    tabLayout->addStretch();

    root->addWidget(m_tabBar);

    // Thin separator after tabs
    m_tabSep = new QFrame;
    m_tabSep->setFrameShape(QFrame::HLine);
    m_tabSep->setStyleSheet("background:#21262d;max-height:1px;");
    root->addWidget(m_tabSep);

    // ── Monitor bar ──────────────────────────────────────────
    m_monitorBar = new MonitorBar(this);
    m_monitorBar->setFixedHeight(180);
    m_monitorBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_monitorBar->setNoMonitorsText(m_s.noMonitors);
    connect(m_monitorBar, &MonitorBar::monitorClicked, this, &MainWindow::onMonitorClicked);
    root->addWidget(m_monitorBar);

    // ── Settings group ────────────────────────────────────────
    m_settingsGroup = new QGroupBox(m_s.groupTitle);
    m_settingsGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QVBoxLayout *sg = new QVBoxLayout(m_settingsGroup);
    sg->setSpacing(10);
    sg->setContentsMargins(12,16,12,12);
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
        m_mediaModeLabel->setStyleSheet("color:#8b949e;font-size:12px;");
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

    // 5. Rotation
    m_rotRow = new QWidget;
    {
        QHBoxLayout *row = new QHBoxLayout(m_rotRow);
        row->setContentsMargins(0,0,0,0); row->setSpacing(8);
        m_rotLabel = new QLabel(m_s.rotLabel);
        m_rotLabel->setStyleSheet("color:#8b949e;font-size:12px;");
        m_rotCombo = new QComboBox;
        m_rotCombo->addItems(m_s.imgRotModes);
        connect(m_rotCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::onRotationChanged);
        row->addWidget(m_rotLabel);
        row->addWidget(m_rotCombo, 1);
    }
    sg->addWidget(m_rotRow);

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
        connect(m_volumeSlider, &QSlider::sliderReleased, this, [this](){
            applyAndSaveCurrent();
        });
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

    // Initialize tab styles (must be after m_settingsGroup creation)
    switchTab(0);

    // Hide tab bar if same wallpaper mode is active
    if (m_sameWallpaper) {
        m_tabBar->setVisible(false);
        m_tabSep->setVisible(false);
    }

    // Gallery — outside settings group so it can stretch to window bottom
    buildGalleryPanel(root);
}

// ============================================================
// Gallery panel  (QListWidget + IconMode)
// ============================================================
void MainWindow::buildGalleryPanel(QVBoxLayout *parent)
{
    m_galleryGroup = new QGroupBox(m_s.galleryTitle);
    m_galleryGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *vl = new QVBoxLayout(m_galleryGroup);
    vl->setSpacing(8);
    vl->setContentsMargins(10,16,10,10);

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
    m_galleryList->setResizeMode(QListView::Adjust);
    m_galleryList->setMovement(QListView::Static);
    m_galleryList->setUniformItemSizes(false);
    m_galleryList->setSpacing(4);
    m_galleryList->setWordWrap(false);
    m_galleryList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_galleryList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_galleryList->setMinimumHeight(m_gridH + 10);
    m_galleryList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_galleryList->setItemDelegate(new GalleryDelegate(m_galleryList));
    m_galleryList->setSelectionMode(QAbstractItemView::NoSelection);
    m_galleryList->viewport()->installEventFilter(this);
    m_galleryList->viewport()->setObjectName("galleryViewport");
    m_galleryList->viewport()->setMouseTracking(true);
    m_galleryList->setMouseTracking(true);

    vl->addWidget(m_galleryList);

    m_galleryEmptyLbl = new QLabel(m_s.galleryEmptyHint);
    m_galleryEmptyLbl->setAlignment(Qt::AlignCenter);
    m_galleryEmptyLbl->setObjectName("galleryEmpty");
    m_galleryEmptyLbl->hide();
    vl->addWidget(m_galleryEmptyLbl);

    parent->addWidget(m_galleryGroup);

    // Watch gallery directory for external changes
    m_galleryWatcher = new QFileSystemWatcher(this);
    QString gDir = ConfigManager::instance().galleryDir();
    m_galleryWatcher->addPath(gDir);
    connect(m_galleryWatcher, &QFileSystemWatcher::directoryChanged,
            this, &MainWindow::refreshGallery);

    recalcGalleryLayout();
    refreshGallery();
}

// ============================================================
// recalcGalleryLayout — adaptive columns based on available width
// ============================================================
void MainWindow::recalcGalleryLayout()
{
    if (!m_galleryList) return;
    int availW = m_galleryList->viewport()->width();
    if (availW <= 0) availW = m_galleryList->width();
    if (availW <= 0) return;

    const int minThumbW = 90;
    const int maxThumbW = 160;
    const int spacing   = 3;
    const int pad       = GRID_PAD;

    // Try to fit 3-8 columns, pick the thumb width that fills the row best
    int bestCols = 3;
    int bestThumbW = minThumbW;
    for (int cols = 3; cols <= 8; ++cols) {
        int cellW = (availW - (cols - 1) * spacing) / cols;
        int tw = cellW - pad;
        if (tw >= minThumbW && tw <= maxThumbW) {
            bestCols = cols;
            bestThumbW = tw;
        }
    }

    m_thumbW = bestThumbW;
    m_thumbH = qRound(m_thumbW * 9.0 / 16.0);  // keep ~16:9
    m_gridW  = m_thumbW + pad;
    m_gridH  = m_thumbH + LABEL_H;

    m_galleryList->setIconSize(QSize(m_thumbW, m_thumbH));
    m_galleryList->setGridSize(QSize(m_gridW, m_gridH));
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

    // Prune stale disk cache entries
    QSet<QString> validPaths;
    for (const GalleryItem &it : items)
        if (!it.isVideo) validPaths.insert(it.path);
    ThumbCache::prune(validPaths);
    ImageCache::prune(validPaths);

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
        wi->setData(Qt::UserRole + 3, m_gridW);
        wi->setData(Qt::UserRole + 4, m_gridH);
        wi->setText(QFileInfo(item.path).fileName());
        wi->setToolTip(item.path);
        wi->setSizeHint(QSize(m_gridW, m_gridH));
        wi->setFlags(Qt::ItemIsEnabled);

        if (item.isVideo && !WallpaperApplier::isGifFile(item.path)) {
            // Video placeholder — build synchronously (trivial cost)
            QPixmap vp(m_thumbW, m_thumbH);
            vp.fill(QColor(16, 10, 30));
            QPainter pp(&vp);
            pp.setPen(QColor(139, 92, 246));
            pp.setFont(QFont("sans", m_thumbH / 4));
            pp.drawText(vp.rect(), Qt::AlignCenter, "\u25b6");
            wi->setIcon(QIcon(vp));
        } else {
            // Check in-memory cache first
            if (m_thumbCache.contains(item.path)) {
                wi->setIcon(QIcon(m_thumbCache[item.path]));
            } else {
                // Try disk cache — instant if hit
                QPixmap cached = ThumbCache::load(item.path, m_thumbW, m_thumbH);
                if (!cached.isNull()) {
                    m_thumbCache[item.path] = cached;
                    wi->setIcon(QIcon(cached));
                } else {
                    // Leave icon empty (placeholder drawn by delegate), load async
                    loadThumbAsync(item.path, m_thumbGeneration);
                }
            }
        }

        m_galleryList->addItem(wi);
    }
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

    // The worker: load + scale entirely off the GUI thread, save to disk cache
    int tw = m_thumbW, th = m_thumbH;
    // Use compressed copy as source for thumbnail generation (much faster I/O)
    QString srcPath = ImageCache::getCompressedOrOriginal(path);
    QFuture<QPixmap> future = QtConcurrent::run([srcPath, path, tw, th]() -> QPixmap {
        // Ensure compressed copy exists (creates JPEG q70 if missing)
        ImageCache::ensureCompressed(path);
        QPixmap thumb = ThumbCache::loadScaled(srcPath, tw, th);
        if (thumb.isNull()) return {};
        ThumbCache::save(path, tw, th, 0, 0, thumb);
        return thumb;
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

    auto &cm = ConfigManager::instance();

    if (m_lockScreenMode) {
        // Lock screen mode — save to hyprlock config
        const WallpaperConfig &hlCfg = m_hyprlockPending[m_currentMonitor];
        cm.setHyprlockConfig(m_currentMonitor, hlCfg);
        cm.save();
        cm.writeHyprlockConf();

        m_monitorBar->setMonitorMode(m_currentMonitor, 0,
                                     hlCfg.filePath.isEmpty() ? QString() : hlCfg.filePath);
    } else {
        // Desktop mode — existing behavior
        const WallpaperConfig &cfg = m_pending[m_currentMonitor];
        const MonitorSlideshowState &ss = m_ssState[m_currentMonitor];

        cm.setConfig(m_currentMonitor, cfg);
        cm.save();

        // Sync hyprlock if same wallpaper mode (only images, not videos)
        if (m_sameWallpaper && !cfg.filePath.isEmpty() &&
            (!WallpaperApplier::isVideoFile(cfg.filePath) || WallpaperApplier::isGifFile(cfg.filePath))) {
            WallpaperConfig hlCfg;
            hlCfg.monitorName = m_currentMonitor;
            hlCfg.filePath = cfg.filePath;
            m_hyprlockPending[m_currentMonitor] = hlCfg;
            cm.setHyprlockConfig(m_currentMonitor, hlCfg);
            cm.writeHyprlockConf();
        }

        if (ss.enabled) {
            startSlideshowForMonitor(m_currentMonitor);
            m_monitorBar->setMonitorMode(m_currentMonitor, 2, {});
        } else {
            stopSlideshowForMonitor(m_currentMonitor);
            if (!cfg.filePath.isEmpty())
                WallpaperApplier::apply(cfg);
            bool gif = !cfg.filePath.isEmpty() && WallpaperApplier::isGifFile(cfg.filePath);
            bool vid = !gif && WallpaperApplier::isVideoFile(cfg.filePath);
            m_monitorBar->setMonitorMode(m_currentMonitor, gif?0:(vid?1:0),
                                         gif ? cfg.filePath : (vid ? QString() : cfg.filePath),
                                         static_cast<int>(cfg.fillMode), static_cast<int>(cfg.rotation));
        }
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
    int pr = m_rotCombo->currentIndex();
    m_rotCombo->blockSignals(true);
    m_rotCombo->clear();
    if (isVideo) { m_rotCombo->addItems(m_s.vidRotModes); }
    else         { m_rotCombo->addItems(m_s.imgRotModes); }
    m_rotCombo->setCurrentIndex(std::min(pr, m_rotCombo->count()-1));
    m_rotCombo->blockSignals(false);
}

void MainWindow::retranslateUi()
{
    setWindowTitle(m_s.windowTitle);
    m_langLabel->setText(m_s.langLabel);
    m_monitorBar->setNoMonitorsText(m_s.noMonitors);
    m_settingsGroup->setTitle(m_s.groupTitle);
    m_audioCheck->setText(m_s.audioCheck);
    m_volumeLabelW->setText(m_s.volumeLabel);
    m_rotLabel->setText(m_s.rotLabel);
    m_bindPrefixLabel->setText(m_s.bindPrefix);
    m_autostartLabel->setText(m_s.autostartLabel);
    m_sameWallpaperLabel->setText(m_s.sameWallpaperLabel);
    m_tabDesktopBtn->setText(m_s.tabDesktop);
    m_tabLockBtn->setText(m_s.tabLockScreen);
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

    int ri = m_rotCombo->currentIndex();
    m_rotCombo->blockSignals(true);
    m_rotCombo->clear();
    if (m_isVideo) { m_rotCombo->addItems(m_s.vidRotModes); }
    else           { m_rotCombo->addItems(m_s.imgRotModes); }
    m_rotCombo->setCurrentIndex(std::min(ri, m_rotCombo->count()-1));
    m_rotCombo->blockSignals(false);

    if (!m_currentMonitor.isEmpty()) populateSettings(m_currentMonitor);
    m_monitorBar->update();
}

void MainWindow::onLanguageChanged(int idx)
{
    m_isRU = (idx == 1);
    m_s = m_isRU ? stringsRU() : stringsEN();
    ConfigManager::instance().saveLanguage(idx);
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

        // Load hyprlock config
        WallpaperConfig hlCfg = cm.getHyprlockConfig(m.name);
        hlCfg.monitorName = m.name;
        m_hyprlockPending[m.name] = hlCfg;

        MonitorSlideshowState &ss = m_ssState[m.name];
        ss.enabled      = cfg.slideshowEnabled;
        ss.intervalSecs = cfg.slideshowInterval;
        ss.mediaMode    = cfg.slideshowMode;
        if (ss.enabled) {
            startSlideshowForMonitor(m.name);
            m_monitorBar->setMonitorMode(m.name, 2, {});
        } else if (!cfg.filePath.isEmpty()) {
            bool gif = !cfg.filePath.isEmpty() && WallpaperApplier::isGifFile(cfg.filePath);
            bool vid = !gif && WallpaperApplier::isVideoFile(cfg.filePath);
            m_monitorBar->setMonitorMode(m.name, gif?0:(vid?1:0),
                                         gif ? cfg.filePath : (vid ? QString() : cfg.filePath),
                                         static_cast<int>(cfg.fillMode), static_cast<int>(cfg.rotation));
        }
    }
    if (!m_monitors.isEmpty()) onMonitorClicked(m_monitors.first().name);

    // Apply same wallpaper initial state
    if (m_sameWallpaper)
        syncSameWallpaper();
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

    WallpaperConfig cfg;
    if (m_lockScreenMode) {
        cfg = m_hyprlockPending.contains(monitorName)
            ? m_hyprlockPending[monitorName]
            : ConfigManager::instance().getHyprlockConfig(monitorName);
    } else {
        cfg = m_pending.contains(monitorName)
            ? m_pending[monitorName]
            : ConfigManager::instance().getConfig(monitorName);
    }
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
    switchToVideo(isVid);

    m_rotCombo->blockSignals(true);
    m_rotCombo->setCurrentIndex(static_cast<int>(cfg.rotation));
    m_audioCheck->setChecked(cfg.audioEnabled);
    m_volumeSlider->setValue(cfg.audioVolume);
    m_volumeLabel->setText(QString("%1%").arg(cfg.audioVolume));
    m_rotCombo->blockSignals(false);

    updateSlideshowDependentWidgets(ss.enabled);
    if (!ss.enabled) {
        m_updatingControls = true; // prevent signal loops during animation setup
        if (isVid) {
            animateSectionShow(m_audioRow);
            if (cfg.audioEnabled) animateSectionShow(m_volumeRow);
            else { m_volumeRow->hide(); }
            m_bindHint->setText(bindString());
            animateSectionShow(m_bindRow);
        } else {
            m_audioRow->hide();
            m_volumeRow->hide();
            m_bindRow->hide();
        }
        m_updatingControls = false;
    }

    // Show correct indicator: slideshow=2, video=1, image=0
    if (ss.enabled)
        m_monitorBar->setMonitorMode(monitorName, 2, {});
    else if (m_lockScreenMode)
        m_monitorBar->setMonitorMode(monitorName, 0,
                                     cfg.filePath.isEmpty() ? QString() : cfg.filePath,
                                     0, 0);
    else {
        bool gif = !cfg.filePath.isEmpty() && WallpaperApplier::isGifFile(cfg.filePath);
        m_monitorBar->setMonitorMode(monitorName, gif?0:(isVid?1:0),
                                     gif ? cfg.filePath : (isVid ? QString() : cfg.filePath),
                                     static_cast<int>(cfg.fillMode), static_cast<int>(cfg.rotation));
    }

    m_updatingControls = false;
}

void MainWindow::saveCurrentToPending()
{
    if (m_currentMonitor.isEmpty()) return;
    WallpaperConfig cfg;
    cfg.monitorName  = m_currentMonitor;

    if (m_lockScreenMode) {
        cfg.filePath     = m_hyprlockPending.contains(m_currentMonitor)
                           ? m_hyprlockPending[m_currentMonitor].filePath : QString();
        cfg.fillMode     = FillMode::Cover;
        cfg.rotation     = static_cast<WallpaperRotation>(m_rotCombo->currentIndex());
        m_hyprlockPending[m_currentMonitor] = cfg;
    } else {
        cfg.filePath     = m_pending.contains(m_currentMonitor)
                           ? m_pending[m_currentMonitor].filePath : QString();
        cfg.fillMode     = FillMode::Cover;
        cfg.rotation     = static_cast<WallpaperRotation>(m_rotCombo->currentIndex());
        cfg.audioEnabled = m_audioCheck->isChecked();
        cfg.audioVolume  = m_volumeSlider->value();
        const MonitorSlideshowState &ss = m_ssState[m_currentMonitor];
        cfg.slideshowEnabled  = ss.enabled;
        cfg.slideshowInterval = ss.intervalSecs;
        cfg.slideshowMode     = ss.mediaMode;
        m_pending[m_currentMonitor] = cfg;
    }
}

void MainWindow::onApplyAll()
{
    saveCurrentToPending();
    auto &cm = ConfigManager::instance();
    for (auto it = m_pending.cbegin(); it != m_pending.cend(); ++it)
        cm.setConfig(it.key(), it.value());
    cm.save();

    // Sync hyprlock if same wallpaper mode
    if (m_sameWallpaper)
        syncSameWallpaper();

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
            bool gif = !it.value().filePath.isEmpty() && WallpaperApplier::isGifFile(it.value().filePath);
            bool vid = !gif && WallpaperApplier::isVideoFile(it.value().filePath);
            m_monitorBar->setMonitorMode(mon, gif?0:(vid?1:0),
                                         gif ? it.value().filePath : (vid ? QString() : it.value().filePath),
                                         static_cast<int>(it.value().fillMode),
                                         static_cast<int>(it.value().rotation));
        }
    }
}

void MainWindow::onGalleryAdd()
{
    QString title  = m_isRU ? QString("\u0414\u043e\u0431\u0430\u0432\u0438\u0442\u044c \u0432 \u0433\u0430\u043b\u0435\u0440\u0435\u044e")
                             : QString("Add to gallery");
    QString filter = m_isRU ? QString("\u0424\u043e\u0442\u043e \u0438 \u0432\u0438\u0434\u0435\u043e")
                             : QString("Images and video");
    filter += " (*.jpg *.jpeg *.png *.bmp *.webp *.tiff *.gif *.mp4 *.mkv *.avi *.webm *.mov *.flv *.wmv);;";
    filter += m_isRU ? QString("\u0412\u0441\u0435 \u0444\u0430\u0439\u043b\u044b (*)")
                     : QString("All files (*)");
    QStringList paths = QFileDialog::getOpenFileNames(this, title, smartBrowseDir(), filter);
    if (paths.isEmpty()) return;
    QList<GalleryItem> added = ConfigManager::instance().addToGallery(paths);
    // Pre-create compressed copies in background
    for (const GalleryItem &item : added) {
        if (!item.isVideo || WallpaperApplier::isGifFile(item.path)) {
            (void)QtConcurrent::run([path = item.path]() {
                ImageCache::ensureCompressed(path);
            });
        }
    }
    refreshGallery();
}

void MainWindow::onGalleryRemove(const QString &path)
{
    // Evict from cache when item is deleted
    m_thumbCache.remove(path);
    ImageCache::removeCompressed(path);
    ConfigManager::instance().removeFromGallery(path);
    refreshGallery();
    if (!m_lockScreenMode && m_pending.contains(m_currentMonitor) &&
        m_pending[m_currentMonitor].filePath == path) {
        m_pending[m_currentMonitor].filePath.clear();
        m_monitorBar->setMonitorMode(m_currentMonitor, -1, {});
    }
    if (m_lockScreenMode && m_hyprlockPending.contains(m_currentMonitor) &&
        m_hyprlockPending[m_currentMonitor].filePath == path) {
        m_hyprlockPending[m_currentMonitor].filePath.clear();
        m_monitorBar->setMonitorMode(m_currentMonitor, -1, {});
    }
}

void MainWindow::onGalleryItemClicked(const QString &path, bool isVideo)
{
    if (m_currentMonitor.isEmpty()) return;

    if (m_lockScreenMode) {
        // Lock screen mode — set hyprlock background
        if (isVideo && !WallpaperApplier::isGifFile(path)) return; // Lock screen doesn't support video (GIF uses first-frame JPEG)
        if (WallpaperApplier::isGifFile(path)) ImageCache::ensureCompressed(path);
        WallpaperConfig cfg;
        cfg.monitorName = m_currentMonitor;
        cfg.filePath = path;
        m_hyprlockPending[m_currentMonitor] = cfg;
        auto &cm = ConfigManager::instance();
        cm.setHyprlockConfig(m_currentMonitor, cfg);
        cm.save();
        cm.writeHyprlockConf();
        m_monitorBar->setMonitorMode(m_currentMonitor, 0, path);
        return;
    }

    // Desktop mode — existing behavior
    if (!m_pending.contains(m_currentMonitor)) {
        WallpaperConfig cfg; cfg.monitorName = m_currentMonitor;
        m_pending[m_currentMonitor] = cfg;
    }
    m_pending[m_currentMonitor].filePath = path;
    switchToVideo(isVideo);
    bool gif = WallpaperApplier::isGifFile(path);
    bool ssOn = m_ssState.value(m_currentMonitor).enabled;
    if (!ssOn) {
        if (isVideo) {
            animateSectionShow(m_audioRow);
            if (m_audioCheck->isChecked()) animateSectionShow(m_volumeRow);
            else m_volumeRow->hide();
            m_bindHint->setText(bindString());
            animateSectionShow(m_bindRow);
        } else {
            animateSectionHide(m_audioRow);
            animateSectionHide(m_volumeRow);
            animateSectionHide(m_bindRow);
        }
    }
    // When slideshow is on, item clicks don't change the bar indicator
    if (!ssOn)
        m_monitorBar->setMonitorMode(m_currentMonitor, gif?0:(isVideo?1:0),
                                     gif ? path : (isVideo ? QString() : path),
                                     static_cast<int>(m_pending[m_currentMonitor].fillMode),
                                     static_cast<int>(m_pending[m_currentMonitor].rotation));
    applyAndSaveCurrent();

    // Sync hyprlock if same wallpaper mode
    if (m_sameWallpaper && (!isVideo || WallpaperApplier::isGifFile(path))) {
        WallpaperConfig hlCfg;
        hlCfg.monitorName = m_currentMonitor;
        hlCfg.filePath = path;
        m_hyprlockPending[m_currentMonitor] = hlCfg;
        auto &cm = ConfigManager::instance();
        cm.setHyprlockConfig(m_currentMonitor, hlCfg);
        cm.save();
        cm.writeHyprlockConf();
    }
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
        if (isVid) {
            animateSectionShow(m_audioRow);
            if (m_audioCheck->isChecked()) animateSectionShow(m_volumeRow);
            m_bindHint->setText(bindString());
            animateSectionShow(m_bindRow);
        } else {
            animateSectionHide(m_audioRow);
            animateSectionHide(m_volumeRow);
            animateSectionHide(m_bindRow);
        }
    }
}

void MainWindow::onRotationChanged(int)
{
    applyAndSaveCurrent();
}

void MainWindow::onAudioToggled(bool checked)
{
    if (checked) animateSectionShow(m_volumeRow);
    else animateSectionHide(m_volumeRow);
    applyAndSaveCurrent();
}

void MainWindow::onVolumeChanged(int val)
{
    m_volumeLabel->setText(QString("%1%").arg(val));
}

void MainWindow::switchTab(int tab)
{
    m_activeTab = tab;
    m_lockScreenMode = (tab == 1);

    // Style tabs
    auto styleTab = [](QPushButton *btn, bool active) {
        if (active) {
            btn->setStyleSheet(
                "QPushButton{background:transparent;border:none;"
                "border-bottom:2px solid #58a6ff;color:#e6edf3;"
                "padding:6px 16px;font-weight:600;font-size:12px;"
                "min-height:22px;max-height:22px;}");
        } else {
            btn->setStyleSheet(
                "QPushButton{background:transparent;border:none;"
                "border-bottom:2px solid transparent;color:#8b949e;"
                "padding:6px 16px;font-size:12px;"
                "min-height:22px;max-height:22px;}"
                "QPushButton:hover{color:#c9d1d9;}");
        }
    };

    styleTab(m_tabDesktopBtn, tab == 0);
    styleTab(m_tabLockBtn, tab == 1);

    // Update settings group title
    m_settingsGroup->setTitle(m_lockScreenMode ? m_s.lockScreenGroupTitle : m_s.groupTitle);

    // Always show settings group
    m_settingsGroup->show();

    if (m_lockScreenMode) {
        // Update monitor bar to show hyprlock wallpapers
        for (const MonitorInfo &m : m_monitors) {
            const WallpaperConfig &hlCfg = m_hyprlockPending.value(m.name);
            if (!hlCfg.filePath.isEmpty())
                m_monitorBar->setMonitorMode(m.name, 0, hlCfg.filePath);
            else
                m_monitorBar->setMonitorMode(m.name, -1, {});
        }
    }

    if (!m_currentMonitor.isEmpty())
        populateSettings(m_currentMonitor);
}

void MainWindow::onSameWallpaperToggled(bool checked)
{
    m_sameWallpaper = checked;
    ConfigManager::instance().saveSameWallpaper(checked);

    // Hide/show tab bar
    m_tabBar->setVisible(!checked);
    m_tabSep->setVisible(!checked);

    if (checked) {
        // Sync mode: switch to desktop tab, apply same wallpaper
        switchTab(0);
        syncSameWallpaper();
    m_settingsGroup->setTitle(m_lockScreenMode ? m_s.lockScreenGroupTitle : m_s.groupTitle);
    } else {
        // Separate mode: restore active tab
        switchTab(m_activeTab);
    }
}

void MainWindow::syncSameWallpaper()
{
    // Copy desktop wallpaper paths to hyprlock configs for all monitors
    auto &cm = ConfigManager::instance();
    for (auto it = m_pending.cbegin(); it != m_pending.cend(); ++it) {
        WallpaperConfig hlCfg;
        hlCfg.monitorName = it.key();
        // Only sync image files (hyprlock doesn't support video)
        if (!it.value().filePath.isEmpty() &&
            (!WallpaperApplier::isVideoFile(it.value().filePath) || WallpaperApplier::isGifFile(it.value().filePath))) {
            hlCfg.filePath = it.value().filePath;
        }
        m_hyprlockPending[it.key()] = hlCfg;
        cm.setHyprlockConfig(it.key(), hlCfg);
    }
    cm.save();
    cm.writeHyprlockConf();

    // Update monitor bar if on lock screen tab
    if (m_lockScreenMode) {
        for (const MonitorInfo &m : m_monitors) {
            const WallpaperConfig &hlCfg = m_hyprlockPending.value(m.name);
            if (!hlCfg.filePath.isEmpty())
                m_monitorBar->setMonitorMode(m.name, 0, hlCfg.filePath);
            else
                m_monitorBar->setMonitorMode(m.name, -1, {});
        }
    }
}

void MainWindow::resizeEvent(QResizeEvent *ev)
{
    QMainWindow::resizeEvent(ev);
    recalcGalleryLayout();
    refreshGallery();
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
    ConfigManager::instance().saveLanguage(m_isRU ? 1 : 0);
    ConfigManager::instance().saveSameWallpaper(m_sameWallpaper);
    QMainWindow::closeEvent(ev);
}

void MainWindow::showAboutDialog()
{
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle(m_isRU ? "О приложении" : "About");
    dlg->setFixedSize(460, 520);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setStyleSheet(
        "QDialog{background:#0d1117;color:#c9d1d9;}"
        "QLabel{color:#c9d1d9;}"
    );

    QVBoxLayout *lay = new QVBoxLayout(dlg);
    lay->setSpacing(10);
    lay->setContentsMargins(24, 20, 24, 16);

    // Title
    QLabel *nameLbl = new QLabel("<h2 style='color:#e6edf3;margin:0;'>HyprWall</h2>");
    nameLbl->setTextFormat(Qt::RichText);
    nameLbl->setAlignment(Qt::AlignCenter);
    lay->addWidget(nameLbl);

    // Version
    QLabel *verLbl = new QLabel("v0.7.0");
    verLbl->setAlignment(Qt::AlignCenter);
    verLbl->setStyleSheet("color:#8b949e;font-size:11px;");
    lay->addWidget(verLbl);

    lay->addSpacing(4);

    // Description
    auto addSection = [&](const QString &title, const QString &text) {
        QLabel *t = new QLabel(QString("<b style='color:#58a6ff;'>%1</b>").arg(title));
        t->setTextFormat(Qt::RichText);
        lay->addWidget(t);
        QLabel *c = new QLabel(text);
        c->setTextFormat(Qt::RichText);
        c->setWordWrap(true);
        c->setStyleSheet("color:#8b949e;font-size:12px;line-height:1.4;");
        lay->addWidget(c);
    };

    if (m_isRU) {
        addSection("\u0427\u0442\u043e \u044d\u0442\u043e?",
            "\u0413\u0440\u0430\u0444\u0438\u0447\u0435\u0441\u043a\u0438\u0439 \u043c\u0435\u043d\u0435\u0434\u0436\u0435\u0440 \u043e\u0431\u043e\u0435\u0432 \u0434\u043b\u044f <b>Hyprland</b>. "
            "\u041f\u043e\u0437\u0432\u043e\u043b\u044f\u0435\u0442 \u043d\u0430\u0441\u0442\u0440\u0430\u0438\u0432\u0430\u0442\u044c \u043e\u0431\u043e\u0438 \u043d\u0430 \u0440\u0430\u0431\u043e\u0447\u0435\u043c \u0441\u0442\u043e\u043b\u0435 \u0438 \u044d\u043a\u0440\u0430\u043d\u0435 \u0431\u043b\u043e\u043a\u0438\u0440\u043e\u0432\u043a\u0438, "
            "\u0443\u043f\u0440\u0430\u0432\u043b\u044f\u0442\u044c \u0441\u043b\u0430\u0439\u0434-\u0448\u043e\u0443\u0430\u043c\u0438 \u0438 \u0433\u0430\u043b\u0435\u0440\u0435\u0435\u0439.");

        addSection("\u0422\u0435\u0445\u043d\u043e\u043b\u043e\u0433\u0438\u0438:",
            "\u2022 <b>C++20 / Qt6</b> \u2014 \u0438\u043d\u0442\u0435\u0440\u0444\u0435\u0439\u0441 \u0438 \u043b\u043e\u0433\u0438\u043a\u0430<br>"
            "\u2022 <b>hyprpaper</b> \u2014 \u0443\u0441\u0442\u0430\u043d\u043e\u0432\u043a\u0430 \u0441\u0442\u0430\u0442\u0438\u0447\u0435\u0441\u043a\u0438\u0445 \u043e\u0431\u043e\u0435\u0432<br>"
            "\u2022 <b>mpvpaper</b> \u2014 \u0432\u0438\u0434\u0435\u043e \u0438 GIF \u043e\u0431\u043e\u0438<br>"
            "\u2022 <b>Wayland</b> (wlroots) \u2014 \u043f\u0440\u043e\u0442\u043e\u043a\u043e\u043b \u0434\u0438\u0441\u043f\u043b\u0435\u044f<br>"
            "\u2022 <b>hyprlock</b> \u2014 \u0443\u043f\u0440\u0430\u0432\u043b\u0435\u043d\u0438\u0435 \u044d\u043a\u0440\u0430\u043d\u043e\u043c \u0431\u043b\u043e\u043a\u0438\u0440\u043e\u0432\u043a\u0438");

        addSection("\u0412\u043e\u0437\u043c\u043e\u0436\u043d\u043e\u0441\u0442\u0438:",
            "\u2022 \u041d\u0430\u0441\u0442\u0440\u043e\u0439\u043a\u0430 \u043e\u0431\u043e\u0435\u0432 \u043d\u0430 \u043a\u0430\u0436\u0434\u043e\u043c \u043c\u043e\u043d\u0438\u0442\u043e\u0440\u0435<br>"
            "\u2022 \u0412\u0438\u0434\u0435\u043e \u0438 GIF \u043e\u0431\u043e\u0438 (mpvpaper)<br>"
            "\u2022 \u0421\u043b\u0430\u0439\u0434-\u0448\u043e\u0443 \u0441 \u043d\u0430\u0441\u0442\u0440\u043e\u0439\u043a\u0430\u043c\u0438<br>"
            "\u2022 \u0421\u0438\u043d\u0445\u0440\u043e\u043d\u0438\u0437\u0430\u0446\u0438\u044f \u0441 \u044d\u043a\u0440\u0430\u043d\u043e\u043c \u0431\u043b\u043e\u043a\u0438\u0440\u043e\u0432\u043a\u0438 (hyprlock)<br>"
            "\u2022 \u0413\u0430\u043b\u0435\u0440\u0435\u044f \u0441 \u043f\u0440\u0435\u0434\u043f\u0440\u043e\u0441\u043c\u043e\u0442\u0440\u043e\u043c<br>"
            "\u2022 \u0410\u0432\u0442\u043e\u0437\u0430\u043f\u0443\u0441\u043a \u043f\u0440\u0438 \u0432\u0445\u043e\u0434\u0435<br>"
            "\u2022 \u0410\u043d\u0433\u043b\u0438\u0439\u0441\u043a\u0438\u0439 / \u0420\u0443\u0441\u0441\u043a\u0438\u0439 \u044f\u0437\u044b\u043a");
    } else {
        addSection("What is this?",
            "A graphical wallpaper manager for <b>Hyprland</b>. "
            "Set wallpapers on your desktop and lock screen, manage slideshows and gallery.");

        addSection("Built with:",
            "\u2022 <b>C++20 / Qt6</b> \u2014 UI and logic<br>"
            "\u2022 <b>hyprpaper</b> \u2014 static image wallpapers<br>"
            "\u2022 <b>mpvpaper</b> \u2014 video and GIF wallpapers<br>"
            "\u2022 <b>Wayland</b> (wlroots) \u2014 display protocol<br>"
            "\u2022 <b>hyprlock</b> \u2014 lock screen management");

        addSection("Features:",
            "\u2022 Per-monitor wallpaper configuration<br>"
            "\u2022 Video and GIF wallpapers (mpvpaper)<br>"
            "\u2022 Slideshow with custom intervals<br>"
            "\u2022 Lock screen sync (hyprlock)<br>"
            "\u2022 Gallery with thumbnail previews<br>"
            "\u2022 Autostart on login<br>"
            "\u2022 English / Russian localization");
    }

    lay->addSpacing(6);

    // Links
    QLabel *linksLbl = new QLabel(
        QString("<a href='https://github.com/Rainkord/HyprWall' style='color:#58a6ff;'>GitHub</a>"
                "&nbsp;&nbsp;|&nbsp;&nbsp;"
                "<a href='https://aur.archlinux.org/packages/hyprwall-git' style='color:#58a6ff;'>AUR</a>"));
    linksLbl->setTextFormat(Qt::RichText);
    linksLbl->setOpenExternalLinks(true);
    linksLbl->setAlignment(Qt::AlignCenter);
    lay->addWidget(linksLbl);

    lay->addStretch();

    // Close button
    QPushButton *okBtn = new QPushButton("OK");
    okBtn->setFixedWidth(80);
    okBtn->setStyleSheet(
        "QPushButton{background:#21262d;color:#c9d1d9;border:1px solid #30363d;"
        "border-radius:6px;padding:6px 12px;font-weight:600;}"
        "QPushButton:hover{background:#30363d;}");
    connect(okBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    lay->addWidget(okBtn, 0, Qt::AlignCenter);

    dlg->exec();
}
