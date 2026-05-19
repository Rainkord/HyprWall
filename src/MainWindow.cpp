#include "MainWindow.h"
#include "MonitorBar.h"
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
#include <QSpinBox>
#include <QFrame>
#include <QDebug>
#include <algorithm>

// ---- Interval table (seconds) matching Strings intervalLabels ----
const int MainWindow::INTERVAL_VALUES[] = { 60, 300, 600, 900, 1800, 3600 };

// ---- Helper: orientation string ----
static QString orientStr(int t, const Strings &s)
{
    switch(t){
        case 1: return s.orientPortrait90;
        case 2: return s.orientLandscape180;
        case 3: return s.orientPortrait270;
        default: return s.orientLandscape;
    }
}

// ---- Autostart helpers (unchanged) ----
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

// ---- Gallery thumbnail helper ----
static QWidget* makeThumb(const GalleryItem &item, MainWindow *mw,
                           const QString &removeTooltip)
{
    // Container
    QWidget *w = new QWidget;
    w->setFixedSize(100, 80);
    w->setCursor(Qt::PointingHandCursor);
    w->setToolTip(QFileInfo(item.path).fileName());
    w->setObjectName("galleryThumb");
    w->setProperty("itemPath",    item.path);
    w->setProperty("itemIsVideo", item.isVideo);

    QVBoxLayout *vl = new QVBoxLayout(w);
    vl->setContentsMargins(0,0,0,0);
    vl->setSpacing(0);

    // Preview
    QLabel *img = new QLabel;
    img->setFixedSize(100, 70);
    img->setAlignment(Qt::AlignCenter);
    img->setScaledContents(false);
    if (!item.isVideo) {
        QPixmap px(item.path);
        if (!px.isNull())
            img->setPixmap(px.scaled(100, 70, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)
                             .copy(0, 0, 100, 70));
        else
            img->setText(QFileInfo(item.path).suffix().toUpper());
    } else {
        img->setText("\u25b6 " + QFileInfo(item.path).suffix().toUpper());
        img->setStyleSheet("background:#1a1a1a; color:#aaa; font-size:12px;");
    }
    vl->addWidget(img);

    // Remove button overlay in top-right
    QPushButton *del = new QPushButton("\u00d7", w);
    del->setFixedSize(18, 18);
    del->move(82, 0);
    del->setObjectName("thumbRemove");
    del->setToolTip(removeTooltip);
    QObject::connect(del, &QPushButton::clicked, [mw, path=item.path]{
        mw->onGalleryRemove(path);
    });

    // Click on thumbnail
    // We install an event filter via a lambda workaround through child label
    img->installEventFilter(mw);
    img->setProperty("itemPath",    item.path);
    img->setProperty("itemIsVideo", item.isVideo);
    img->setObjectName("thumbImg");

    return w;
}

// ---- MainWindow ----

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_s = stringsEN();
    m_slideshowTimer = new QTimer(this);
    connect(m_slideshowTimer, &QTimer::timeout, this, &MainWindow::onSlideshowTick);

    buildUi();
    loadMonitors();

    // Restore slideshow state from config
    ConfigManager::instance().load();
    SlideshowConfig ss = ConfigManager::instance().slideshowConfig();
    m_slideshowCheck->setChecked(ss.enabled);
    // Find matching interval index
    int sIdx = 1; // default 5 min
    for (int i = 0; i < 6; ++i) {
        if (INTERVAL_VALUES[i] == ss.intervalSecs) { sIdx = i; break; }
    }
    m_intervalCombo->setCurrentIndex(sIdx);
    if (ss.enabled) {
        m_intervalCombo->setEnabled(true);
        m_slideshowTimer->start(ss.intervalSecs * 1000);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonRelease) {
        QLabel *lbl = qobject_cast<QLabel*>(obj);
        if (lbl && lbl->objectName() == "thumbImg") {
            QString path    = lbl->property("itemPath").toString();
            bool    isVideo = lbl->property("itemIsVideo").toBool();
            onGalleryItemClicked(path, isVideo);
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, ev);
}

void MainWindow::buildUi()
{
    setWindowTitle(m_s.windowTitle);
    setMinimumWidth(420);

    QWidget *central = new QWidget;
    setCentralWidget(central);
    QVBoxLayout *root = new QVBoxLayout(central);
    root->setSpacing(8);
    root->setContentsMargins(10,10,10,10);

    // ---- Top bar: lang + autostart ----
    {
        QHBoxLayout *top = new QHBoxLayout;
        m_langLabel = new QLabel(m_s.langLabel);
        m_langCombo = new QComboBox;
        m_langCombo->addItem("EN"); m_langCombo->addItem("RU");
        connect(m_langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::onLanguageChanged);
        m_autostartLabel = new QLabel(m_s.autostartLabel);
        m_autostartBtn   = new QPushButton;
        connect(m_autostartBtn, &QPushButton::clicked, this, &MainWindow::onAutostartToggle);
        updateAutostartButton();
        top->addWidget(m_langLabel);
        top->addWidget(m_langCombo);
        top->addStretch();
        top->addWidget(m_autostartLabel);
        top->addWidget(m_autostartBtn);
        root->addLayout(top);
    }

    // ---- Monitor bar ----
    m_monitorBar = new MonitorBar(this);
    connect(m_monitorBar, &MonitorBar::monitorClicked, this, &MainWindow::onMonitorClicked);
    root->addWidget(m_monitorBar);

    // ---- Settings group ----
    m_settingsGroup = new QGroupBox(m_s.groupTitle);
    QVBoxLayout *sg = new QVBoxLayout(m_settingsGroup);
    sg->setSpacing(6);

    // Orientation info
    m_orientationLabel = new QLabel;
    m_orientationLabel->setObjectName("orientLabel");
    sg->addWidget(m_orientationLabel);

    // File row
    {
        QHBoxLayout *row = new QHBoxLayout;
        m_fileLabel = new QLabel(m_s.fileLabel);
        m_fileEdit  = new QLineEdit;
        m_fileEdit->setReadOnly(true);
        m_browseBtn = new QPushButton(m_s.browseBtn);
        connect(m_browseBtn, &QPushButton::clicked, this, &MainWindow::onBrowseFile);
        row->addWidget(m_fileLabel);
        row->addWidget(m_fileEdit, 1);
        row->addWidget(m_browseBtn);
        sg->addLayout(row);
    }

    // Audio row
    {
        QWidget *audioRow = new QWidget;
        QHBoxLayout *row = new QHBoxLayout(audioRow);
        row->setContentsMargins(0,0,0,0);
        m_audioCheck = new QCheckBox(m_s.audioCheck);
        m_audioCheck->setProperty("rowWidget", QVariant::fromValue(audioRow));
        connect(m_audioCheck, &QCheckBox::toggled, this, &MainWindow::onAudioToggled);
        row->addWidget(m_audioCheck);
        row->addStretch();
        sg->addWidget(audioRow);
        audioRow->hide();
    }

    // Volume row
    {
        QWidget *volRow = new QWidget;
        QHBoxLayout *row = new QHBoxLayout(volRow);
        row->setContentsMargins(0,0,0,0);
        m_volumeLabelW = new QLabel(m_s.volumeLabel);
        m_volumeSlider = new QSlider(Qt::Horizontal);
        m_volumeSlider->setRange(0, 100);
        m_volumeSlider->setValue(50);
        m_volumeSlider->setProperty("volWidget", QVariant::fromValue(volRow));
        m_volumeLabel = new QLabel("50%");
        connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
        row->addWidget(m_volumeLabelW);
        row->addWidget(m_volumeSlider, 1);
        row->addWidget(m_volumeLabel);
        sg->addWidget(volRow);
        volRow->hide();
    }

    // Fill row
    {
        QHBoxLayout *row = new QHBoxLayout;
        m_fillLabel = new QLabel(m_s.fillLabel);
        m_fillCombo = new QComboBox;
        m_fillCombo->addItems(m_s.imgFillModes);
        connect(m_fillCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::onFillModeChanged);
        row->addWidget(m_fillLabel);
        row->addWidget(m_fillCombo, 1);
        sg->addLayout(row);
    }

    // Rotation row
    {
        QHBoxLayout *row = new QHBoxLayout;
        m_rotLabel = new QLabel(m_s.rotLabel);
        m_rotCombo = new QComboBox;
        m_rotCombo->addItems(m_s.imgRotModes);
        connect(m_rotCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::onRotationChanged);
        row->addWidget(m_rotLabel);
        row->addWidget(m_rotCombo, 1);
        sg->addLayout(row);
    }

    // ---- Gallery panel (between rotation and apply) ----
    buildGalleryPanel(sg);

    // ---- Slideshow panel ----
    buildSlideshowPanel(sg);

    // Bind hint row
    {
        m_bindRow = new QWidget;
        QHBoxLayout *row = new QHBoxLayout(m_bindRow);
        row->setContentsMargins(0,0,0,0);
        m_bindPrefixLabel = new QLabel(m_s.bindPrefix);
        m_bindHint = new QLabel;
        m_bindHint->setObjectName("bindHint");
        m_bindHint->setTextInteractionFlags(Qt::TextSelectableByMouse);
        row->addWidget(m_bindPrefixLabel);
        row->addWidget(m_bindHint, 1);
        sg->addWidget(m_bindRow);
        m_bindRow->hide();
    }

    // Apply button
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

    // Top bar: title + add button
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
    scroll->setFixedHeight(200);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

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

void MainWindow::buildSlideshowPanel(QVBoxLayout *parent)
{
    m_slideshowRow = new QWidget;
    QHBoxLayout *row = new QHBoxLayout(m_slideshowRow);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(6);

    m_slideshowCheck = new QCheckBox(m_s.slideshowLabel);
    connect(m_slideshowCheck, &QCheckBox::toggled, this, &MainWindow::onSlideshowToggled);

    m_intervalPrefixLbl = new QLabel(m_s.slideshowIntervalLabel);
    m_intervalCombo = new QComboBox;
    m_intervalCombo->addItems(m_s.intervalLabels);
    m_intervalCombo->setCurrentIndex(1); // 5 min default
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

    row->addWidget(m_slideshowCheck);
    row->addWidget(m_intervalPrefixLbl);
    row->addWidget(m_intervalCombo);
    row->addWidget(m_intervalSuffixLbl);
    row->addStretch();

    parent->addWidget(m_slideshowRow);
}

void MainWindow::refreshGallery()
{
    // Clear old grid layout
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
        vl->addWidget(m_galleryEmptyLbl);
        m_galleryEmptyLbl->show();
        return;
    }
    m_galleryEmptyLbl->hide();

    // Flow grid: 4 columns of 100px thumbnails
    QGridLayout *grid = new QGridLayout(m_galleryGrid);
    grid->setSpacing(4);
    grid->setContentsMargins(4,4,4,4);
    int col = 0, row = 0;
    const int COLS = 4;
    for (const GalleryItem &item : items) {
        QWidget *thumb = makeThumb(item, this, m_s.galleryRemoveTooltip);
        grid->addWidget(thumb, row, col);
        ++col;
        if (col >= COLS) { col = 0; ++row; }
    }
}

void MainWindow::updateAutostartButton()
{
    if (m_autostartBtn)
        m_autostartBtn->setText(autostartEnabled() ? m_s.autostartDisable : m_s.autostartEnable);
}

void MainWindow::onAutostartToggle()
{
    setAutostart(!autostartEnabled());
    updateAutostartButton();
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
    m_fileLabel->setText(m_s.fileLabel);
    m_browseBtn->setText(m_s.browseBtn);
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
    {
        int ci = m_intervalCombo->currentIndex();
        m_intervalCombo->blockSignals(true);
        m_intervalCombo->clear();
        m_intervalCombo->addItems(m_s.intervalLabels);
        m_intervalCombo->setCurrentIndex(ci);
        m_intervalCombo->blockSignals(false);
    }
    updateAutostartButton();
    int fi = m_fillCombo->currentIndex();
    int ri = m_rotCombo->currentIndex();
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
    // Prefer ~/Pictures/wallpapers, then ~/Pictures
    QString wp = QDir::homePath() + "/Pictures/wallpapers";
    if (QDir(wp).exists()) return wp;
    QString pics = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (QDir(pics).exists()) return pics;
    return QDir::homePath();
}

void MainWindow::loadMonitors()
{
    m_monitors = MonitorDetector::detect();
    m_monitorBar->setMonitors(m_monitors);
    ConfigManager &cm = ConfigManager::instance();
    cm.load();
    for (const MonitorInfo &m : m_monitors) {
        WallpaperConfig cfg = cm.getConfig(m.name);
        // Pre-fill pending from saved config
        cfg.monitorName = m.name;
        m_pending[m.name] = cfg;
        if (!cfg.filePath.isEmpty()) {
            bool vid = WallpaperApplier::isVideoFile(cfg.filePath);
            m_monitorBar->setMonitorMode(m.name, vid ? 1 : 0, vid ? QString() : cfg.filePath);
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

    // Read from pending (not ConfigManager) so unsaved changes are preserved
    WallpaperConfig cfg;
    if (m_pending.contains(monitorName))
        cfg = m_pending[monitorName];
    else {
        cfg = ConfigManager::instance().getConfig(monitorName);
        cfg.monitorName = monitorName;
    }

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

    QWidget *audioRow = m_audioCheck->property("rowWidget").value<QWidget*>();
    if (audioRow) audioRow->setVisible(isVid);
    m_audioCheck->setVisible(isVid);
    QWidget *vw = m_volumeSlider->property("volWidget").value<QWidget*>();
    if (vw) vw->setVisible(isVid && cfg.audioEnabled);
    if (isVid) { m_bindHint->setText(bindString()); m_bindRow->show(); }
    else m_bindRow->hide();
    m_monitorBar->setMonitorMode(monitorName, isVid ? 1 : 0,
                                 isVid ? QString() : cfg.filePath);
}

void MainWindow::saveCurrentToPending()
{
    if (m_currentMonitor.isEmpty()) return;
    WallpaperConfig cfg;
    cfg.monitorName  = m_currentMonitor;
    cfg.filePath     = m_fileEdit->text();
    cfg.fillMode     = static_cast<FillMode>(m_fillCombo->currentIndex());
    cfg.rotation     = static_cast<WallpaperRotation>(m_rotCombo->currentIndex());
    cfg.audioEnabled = m_audioCheck->isChecked();
    cfg.audioVolume  = m_volumeSlider->value();
    m_pending[m_currentMonitor] = cfg;
}

void MainWindow::onApplyAll()
{
    // Save the currently visible monitor settings into pending first
    saveCurrentToPending();

    // Write all pending configs to ConfigManager and apply all at once
    auto &cm = ConfigManager::instance();
    for (auto it = m_pending.cbegin(); it != m_pending.cend(); ++it)
        cm.setConfig(it.key(), it.value());
    cm.save();

    WallpaperApplier::applyAll(m_pending);

    // Update monitor bar thumbnails
    for (auto it = m_pending.cbegin(); it != m_pending.cend(); ++it) {
        bool vid = WallpaperApplier::isVideoFile(it.value().filePath);
        m_monitorBar->setMonitorMode(it.key(), vid ? 1 : 0,
                                     vid ? QString() : it.value().filePath);
    }
}

void MainWindow::onBrowseFile()
{
    QString title  = m_isRU ? QString("\u0412\u044b\u0431\u0435\u0440\u0438\u0442\u0435 \u0444\u0430\u0439\u043b") : QString("Select file");
    QString filter = m_isRU ? QString("\u0418\u0437\u043e\u0431\u0440\u0430\u0436\u0435\u043d\u0438\u044f \u0438 \u0432\u0438\u0434\u0435\u043e") : QString("Images and video");
    filter += " (*.jpg *.jpeg *.png *.bmp *.gif *.mp4 *.mkv *.avi *.webm *.mov);;";
    filter += m_isRU ? QString("\u0412\u0441\u0435 \u0444\u0430\u0439\u043b\u044b (*)") : QString("All files (*)");
    QString path = QFileDialog::getOpenFileName(this, title, smartBrowseDir(), filter);
    if (path.isEmpty()) return;
    m_fileEdit->setText(path);
    bool isVid = WallpaperApplier::isVideoFile(path);
    switchToVideo(isVid);
    QWidget *audioRow = m_audioCheck->property("rowWidget").value<QWidget*>();
    if (audioRow) audioRow->setVisible(isVid);
    m_audioCheck->setVisible(isVid);
    QWidget *vw = m_volumeSlider->property("volWidget").value<QWidget*>();
    if (vw) vw->setVisible(isVid && m_audioCheck->isChecked());
    if (isVid) { m_bindHint->setText(bindString()); m_bindRow->show(); } else m_bindRow->hide();
    m_monitorBar->setMonitorMode(m_currentMonitor, isVid ? 1 : 0, isVid ? QString() : path);
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
}

void MainWindow::onGalleryItemClicked(const QString &path, bool isVideo)
{
    if (m_currentMonitor.isEmpty()) return;
    m_fileEdit->setText(path);
    switchToVideo(isVideo);
    QWidget *audioRow = m_audioCheck->property("rowWidget").value<QWidget*>();
    if (audioRow) audioRow->setVisible(isVideo);
    m_audioCheck->setVisible(isVideo);
    QWidget *vw = m_volumeSlider->property("volWidget").value<QWidget*>();
    if (vw) vw->setVisible(isVideo && m_audioCheck->isChecked());
    if (isVideo) { m_bindHint->setText(bindString()); m_bindRow->show(); } else m_bindRow->hide();
    m_monitorBar->setMonitorMode(m_currentMonitor, isVideo ? 1 : 0,
                                 isVideo ? QString() : path);
    // Update pending immediately for this monitor
    saveCurrentToPending();
}

void MainWindow::onSlideshowToggled(bool checked)
{
    m_intervalCombo->setEnabled(checked);
    SlideshowConfig ss = ConfigManager::instance().slideshowConfig();
    ss.enabled = checked;
    ss.intervalSecs = INTERVAL_VALUES[m_intervalCombo->currentIndex()];
    ConfigManager::instance().setSlideshowConfig(ss);
    ConfigManager::instance().save();

    if (checked) {
        m_slideshowTimer->start(ss.intervalSecs * 1000);
    } else {
        m_slideshowTimer->stop();
    }
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
    QWidget *vw = m_volumeSlider->property("volWidget").value<QWidget*>();
    if (vw) vw->setVisible(checked);
}

void MainWindow::onVolumeChanged(int val)
{
    m_volumeLabel->setText(QString("%1%").arg(val));
}
