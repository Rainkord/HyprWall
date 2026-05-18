#include "MainWindow.h"
#include "WallpaperApplier.h"
#include "ConfigManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLabel>
#include <QPainter>
#include <QApplication>
#include <QMessageBox>
#include <QFont>
#include <QDir>
#include <QMouseEvent>
#include <QMap>
#include <QPixmap>
#include <algorithm>
#include <climits>

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

// ============================================================
// MonitorBar
// ============================================================
class MonitorBar : public QWidget {
    Q_OBJECT
public:
    explicit MonitorBar(QWidget *p = nullptr) : QWidget(p) {
        setMinimumHeight(160);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setStyleSheet("background:#1a1a2e;border-radius:6px;");
    }
    void setMonitors(const QList<MonitorInfo> &m)  { m_monitors=m; m_selected=m.isEmpty()?"":m.first().name; update(); }
    void setSelected(const QString &n)              { m_selected=n; update(); }
    void setNoMonitorsText(const QString &t)        { m_noMon=t; update(); }
    void setWallpaperPath(const QString &mon, const QString &path) {
        if (path.isEmpty()) return;
        if (!WallpaperApplier::isVideoFile(path)) {
            QPixmap px(path);
            if (!px.isNull()) m_pixmaps[mon]=px;
        } else m_pixmaps.remove(mon);
        m_wallpapers[mon]=path;
        update();
    }
signals:
    void monitorClicked(const QString &name);
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        if (m_monitors.isEmpty()) {
            p.setPen(QColor(120,120,120));
            p.drawText(rect(), Qt::AlignCenter, m_noMon);
            return;
        }
        int mnX=INT_MAX,mnY=INT_MAX,mxX=INT_MIN,mxY=INT_MIN;
        for (auto &m:m_monitors){mnX=std::min(mnX,m.x);mnY=std::min(mnY,m.y);mxX=std::max(mxX,m.x+m.width);mxY=std::max(mxY,m.y+m.height);}
        int tW=mxX-mnX,tH=mxY-mnY; if(!tW||!tH) return;
        const int P=14;
        int aW=width()-2*P,aH=height()-2*P;
        double sc=std::min((double)aW/tW,(double)aH/tH);
        int oX=P+(aW-(int)(tW*sc))/2,oY=P+(aH-(int)(tH*sc))/2;
        for (auto &m:m_monitors){
            int rx=oX+(int)((m.x-mnX)*sc),ry=oY+(int)((m.y-mnY)*sc);
            int rw=std::max(6,(int)(m.width*sc)),rh=std::max(6,(int)(m.height*sc));
            QRect r(rx,ry,rw,rh); bool sel=(m.name==m_selected);
            if (m_pixmaps.contains(m.name))
                p.drawPixmap(r,m_pixmaps[m.name].scaled(r.size(),Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation));
            else if (m_wallpapers.contains(m.name)&&WallpaperApplier::isVideoFile(m_wallpapers[m.name]))
                {p.fillRect(r,QColor(30,20,50));p.setPen(QColor(180,120,255));QFont f=p.font();f.setPointSize(14);p.setFont(f);p.drawText(r,Qt::AlignCenter,">");}
            else p.fillRect(r,QColor(35,35,55));
            p.setPen(QPen(sel?QColor(0,200,255):QColor(80,80,110),sel?3:1));p.setBrush(Qt::NoBrush);p.drawRect(r);
            int lH=std::min(20,rh);QRect lr(rx,ry+rh-lH,rw,lH);
            p.fillRect(lr,QColor(0,0,0,160));p.setPen(Qt::white);
            QFont f=p.font();f.setPointSize(7);f.setBold(sel);p.setFont(f);
            p.drawText(lr,Qt::AlignCenter,m.name);
        }
    }
    void mousePressEvent(QMouseEvent *ev) override {
        if (m_monitors.isEmpty()) return;
        int mnX=INT_MAX,mnY=INT_MAX,mxX=INT_MIN,mxY=INT_MIN;
        for (auto &m:m_monitors){mnX=std::min(mnX,m.x);mnY=std::min(mnY,m.y);mxX=std::max(mxX,m.x+m.width);mxY=std::max(mxY,m.y+m.height);}
        int tW=mxX-mnX,tH=mxY-mnY; if(!tW||!tH) return;
        const int P=14;int aW=width()-2*P,aH=height()-2*P;
        double sc=std::min((double)aW/tW,(double)aH/tH);
        int oX=P+(aW-(int)(tW*sc))/2,oY=P+(aH-(int)(tH*sc))/2;
        for (auto &m:m_monitors){
            int rx=oX+(int)((m.x-mnX)*sc),ry=oY+(int)((m.y-mnY)*sc);
            int rw=std::max(6,(int)(m.width*sc)),rh=std::max(6,(int)(m.height*sc));
            if (QRect(rx,ry,rw,rh).contains(ev->pos())){emit monitorClicked(m.name);return;}
        }
    }
private:
    QList<MonitorInfo> m_monitors;
    QString m_selected,m_noMon{"No monitors"};
    QMap<QString,QString> m_wallpapers;
    QMap<QString,QPixmap> m_pixmaps;
};
#include "MainWindow.moc"

// ============================================================
// MainWindow
// ============================================================
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_s = stringsEN();
    ConfigManager::instance().load();
    buildUi();
    loadMonitors();
}

void MainWindow::buildUi()
{
    setWindowTitle(m_s.windowTitle);
    setMinimumSize(560, 640);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *root = new QVBoxLayout(central);
    root->setSpacing(6);
    root->setContentsMargins(12,10,12,10);

    // language row
    {
        QHBoxLayout *lr = new QHBoxLayout;
        m_langLabel = new QLabel(m_s.langLabel);
        m_langCombo = new QComboBox;
        m_langCombo->addItems({"English", "\u0420\u0443\u0441\u0441\u043a\u0438\u0439"});
        connect(m_langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MainWindow::onLanguageChanged);
        lr->addWidget(m_langLabel);
        lr->addWidget(m_langCombo);
        lr->addStretch();
        root->addLayout(lr);
    }

    // monitor bar
    m_monitorBar = new MonitorBar(this);
    m_monitorBar->setNoMonitorsText(m_s.noMonitors);
    connect(m_monitorBar, &MonitorBar::monitorClicked, this, [this](const QString &name){
        int idx = m_monitorCombo->findText(name);
        if (idx >= 0) m_monitorCombo->setCurrentIndex(idx);
    });
    root->addWidget(m_monitorBar);

    // monitor selector
    {
        QHBoxLayout *row = new QHBoxLayout;
        m_monitorLabel = new QLabel(m_s.monitorLabel);
        m_monitorCombo = new QComboBox;
        row->addWidget(m_monitorLabel);
        row->addWidget(m_monitorCombo, 1);
        root->addLayout(row);
    }

    // settings group
    m_settingsGroup = new QGroupBox(m_s.groupTitle);
    QVBoxLayout *sg = new QVBoxLayout(m_settingsGroup);
    sg->setSpacing(5);

    m_orientationLabel = new QLabel("-");
    m_orientationLabel->setStyleSheet("color:#aaa;font-size:11px;");
    sg->addWidget(m_orientationLabel);

    // file row
    {
        QHBoxLayout *row = new QHBoxLayout;
        m_fileLabel = new QLabel(m_s.fileLabel);
        m_fileEdit = new QLineEdit;
        m_browseBtn = new QPushButton(m_s.browseBtn);
        connect(m_browseBtn, &QPushButton::clicked, this, &MainWindow::onBrowseFile);
        row->addWidget(m_fileLabel);
        row->addWidget(m_fileEdit, 1);
        row->addWidget(m_browseBtn);
        sg->addLayout(row);
    }

    // audio checkbox
    m_audioCheck = new QCheckBox(m_s.audioCheck);
    connect(m_audioCheck, &QCheckBox::toggled, this, &MainWindow::onAudioToggled);
    m_audioCheck->hide();
    sg->addWidget(m_audioCheck);

    // volume row
    {
        QWidget *vw = new QWidget;
        QHBoxLayout *row = new QHBoxLayout(vw);
        row->setContentsMargins(0,0,0,0);
        m_volumeLabelW = new QLabel(m_s.volumeLabel);
        m_volumeSlider = new QSlider(Qt::Horizontal);
        m_volumeSlider->setRange(0,100); m_volumeSlider->setValue(50);
        m_volumeLabel = new QLabel("50%"); m_volumeLabel->setMinimumWidth(36);
        connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
        row->addWidget(m_volumeLabelW);
        row->addWidget(m_volumeSlider, 1);
        row->addWidget(m_volumeLabel);
        vw->hide();
        m_volumeSlider->setProperty("volWidget", QVariant::fromValue(vw));
        sg->addWidget(vw);
    }

    // bind hint (selectable text)
    {
        m_bindRow = new QWidget;
        QVBoxLayout *vl = new QVBoxLayout(m_bindRow);
        vl->setContentsMargins(0,0,0,0); vl->setSpacing(1);
        m_bindPrefixLabel = new QLabel(m_s.bindPrefix);
        m_bindPrefixLabel->setStyleSheet("color:#888;font-size:10px;");
        m_bindHint = new QLabel;
        m_bindHint->setStyleSheet("color:#aaa;font-size:10px;font-family:monospace;");
        m_bindHint->setTextInteractionFlags(Qt::TextSelectableByMouse);
        vl->addWidget(m_bindPrefixLabel);
        vl->addWidget(m_bindHint);
        m_bindRow->hide();
        sg->addWidget(m_bindRow);
    }

    // fill
    {
        QHBoxLayout *row = new QHBoxLayout;
        m_fillLabel = new QLabel(m_s.fillLabel);
        m_fillCombo = new QComboBox;
        m_fillCombo->addItems(m_s.imgFillModes);
        row->addWidget(m_fillLabel);
        row->addWidget(m_fillCombo, 1);
        sg->addLayout(row);
    }

    // rotation
    {
        QHBoxLayout *row = new QHBoxLayout;
        m_rotLabel = new QLabel(m_s.rotLabel);
        m_rotCombo = new QComboBox;
        m_rotCombo->addItems(m_s.imgRotModes);
        row->addWidget(m_rotLabel);
        row->addWidget(m_rotCombo, 1);
        sg->addLayout(row);
    }

    // apply
    m_applyBtn = new QPushButton(m_s.applyBtn);
    m_applyBtn->setFixedHeight(34);
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

    m_fillCombo->blockSignals(true);
    m_rotCombo->blockSignals(true);
    m_fillCombo->clear();
    m_rotCombo->clear();
    if (isVideo) {
        m_fillCombo->addItems(m_s.vidFillModes);
        m_rotCombo->addItems(m_s.vidRotModes);
    } else {
        m_fillCombo->addItems(m_s.imgFillModes);
        m_rotCombo->addItems(m_s.imgRotModes);
    }
    // clamp index to new list size
    m_fillCombo->setCurrentIndex(std::min(prevFill, m_fillCombo->count()-1));
    m_rotCombo->setCurrentIndex(std::min(prevRot,  m_rotCombo->count()-1));
    m_fillCombo->blockSignals(false);
    m_rotCombo->blockSignals(false);
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

    int fi = m_fillCombo->currentIndex();
    int ri = m_rotCombo->currentIndex();
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

    // Switch combo items before setting index
    m_isVideo = !isVid; // force switch
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
