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

// ─── helpers ─────────────────────────────────────────────────────────────────
static QString orientationString(int transform)
{
    // transform: 0=normal(альбомный), 1=90°(книжный), 2=180°(альбомный перевёрнутый)
    //            3=270°(книжный перевёрнутый), 4..7 = flipped варианты
    switch (transform % 4) {
        case 0: return "Альбомный";
        case 1: return "Книжный (90°)";
        case 2: return "Альбомный перевёрнутый (180°)";
        case 3: return "Книжный перевёрнутый (270°)";
        default: return "Неизвестно";
    }
}

// ─── MonitorBar ───────────────────────────────────────────────────────────────
class MonitorBar : public QWidget {
    Q_OBJECT
public:
    explicit MonitorBar(QWidget *p = nullptr) : QWidget(p) {
        setMinimumHeight(150);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setStyleSheet("background: #1a1a2e; border-radius: 6px;");
    }

    void setMonitors(const QList<MonitorInfo> &monitors) {
        m_monitors = monitors;
        m_selected = monitors.isEmpty() ? "" : monitors.first().name;
        update();
    }

    void setSelected(const QString &name) {
        m_selected = name;
        update();
    }

    void setWallpaperPath(const QString &monitor, const QString &path) {
        if (path.isEmpty()) return;
        // кэшируем пиксмап для превью
        if (!WallpaperApplier::isVideoFile(path)) {
            QPixmap px(path);
            if (!px.isNull())
                m_pixmaps[monitor] = px;
        } else {
            m_pixmaps.remove(monitor); // для видео — заглушка
        }
        m_wallpapers[monitor] = path;
        update();
    }

signals:
    void monitorClicked(const QString &name);

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        if (m_monitors.isEmpty()) {
            p.setPen(QColor(120,120,120));
            p.drawText(rect(), Qt::AlignCenter, "Мониторы не найдены");
            return;
        }

        // Вычисляем bounding box всей раскладки мониторов
        int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;
        for (auto &m : m_monitors) {
            minX = std::min(minX, m.x);
            minY = std::min(minY, m.y);
            maxX = std::max(maxX, m.x + m.width);
            maxY = std::max(maxY, m.y + m.height);
        }
        int totalW = maxX - minX;
        int totalH = maxY - minY;
        if (!totalW || !totalH) return;

        const int PAD = 14;
        int avW = width()  - 2*PAD;
        int avH = height() - 2*PAD;
        double sc = std::min((double)avW / totalW, (double)avH / totalH);

        // Центрируем всю раскладку внутри виджета
        int scaledW = (int)(totalW * sc);
        int scaledH = (int)(totalH * sc);
        int offsetX = PAD + (avW - scaledW) / 2;
        int offsetY = PAD + (avH - scaledH) / 2;

        for (auto &m : m_monitors) {
            int rx = offsetX + (int)((m.x - minX) * sc);
            int ry = offsetY + (int)((m.y - minY) * sc);
            int rw = std::max(6, (int)(m.width  * sc));
            int rh = std::max(6, (int)(m.height * sc));
            QRect rect(rx, ry, rw, rh);
            bool sel = (m.name == m_selected);

            // фон / превью обоев
            if (m_pixmaps.contains(m.name)) {
                p.drawPixmap(rect,
                    m_pixmaps[m.name].scaled(rect.size(),
                        Qt::KeepAspectRatioByExpanding,
                        Qt::SmoothTransformation));
            } else if (m_wallpapers.contains(m.name) &&
                       WallpaperApplier::isVideoFile(m_wallpapers[m.name])) {
                // видео — тёмно-фиолетовый с иконкой
                p.fillRect(rect, QColor(30, 20, 50));
                p.setPen(QColor(180,120,255));
                QFont fi = p.font(); fi.setPointSize(16); p.setFont(fi);
                p.drawText(rect, Qt::AlignCenter, "▶");
            } else {
                p.fillRect(rect, QColor(35, 35, 55));
            }

            // рамка
            QPen pen(sel ? QColor(0, 200, 255) : QColor(80, 80, 110), sel ? 3 : 1);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawRect(rect);

            // подложка под текст
            QRect labelRect(rx, ry + rh - 22, rw, 22);
            p.fillRect(labelRect, QColor(0,0,0,140));

            // имя + ориентация
            p.setPen(Qt::white);
            QFont f = p.font();
            f.setPointSize(7); f.setBold(sel);
            p.setFont(f);
            QString label = m.name;
            p.drawText(labelRect, Qt::AlignCenter, label);
        }
    }

    void mousePressEvent(QMouseEvent *ev) override {
        if (m_monitors.isEmpty()) return;

        int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;
        for (auto &m : m_monitors) {
            minX = std::min(minX, m.x); minY = std::min(minY, m.y);
            maxX = std::max(maxX, m.x + m.width);
            maxY = std::max(maxY, m.y + m.height);
        }
        int totalW = maxX - minX, totalH = maxY - minY;
        if (!totalW || !totalH) return;

        const int PAD = 14;
        int avW = width()  - 2*PAD;
        int avH = height() - 2*PAD;
        double sc = std::min((double)avW / totalW, (double)avH / totalH);
        int scaledW = (int)(totalW * sc);
        int scaledH = (int)(totalH * sc);
        int offsetX = PAD + (avW - scaledW) / 2;
        int offsetY = PAD + (avH - scaledH) / 2;

        for (auto &m : m_monitors) {
            int rx = offsetX + (int)((m.x - minX) * sc);
            int ry = offsetY + (int)((m.y - minY) * sc);
            int rw = std::max(6, (int)(m.width  * sc));
            int rh = std::max(6, (int)(m.height * sc));
            if (QRect(rx,ry,rw,rh).contains(ev->pos())) {
                emit monitorClicked(m.name);
                return;
            }
        }
    }

private:
    QList<MonitorInfo>     m_monitors;
    QString                m_selected;
    QMap<QString, QString> m_wallpapers;
    QMap<QString, QPixmap> m_pixmaps;     // кэш превью
};

#include "MainWindow.moc"

// ─── MainWindow ───────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("HyprWall");
    setMinimumSize(560, 640);
    ConfigManager::instance().load();
    buildUi();
    loadMonitors();
}

void MainWindow::buildUi()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *root = new QVBoxLayout(central);
    root->setSpacing(8);
    root->setContentsMargins(12, 12, 12, 12);

    // монитор-бар
    m_monitorBar = new MonitorBar(this);
    connect(m_monitorBar, &MonitorBar::monitorClicked, this, [this](const QString &name){
        int idx = m_monitorCombo->findText(name);
        if (idx >= 0) m_monitorCombo->setCurrentIndex(idx);
    });
    root->addWidget(m_monitorBar);

    // выбор монитора
    QHBoxLayout *comboRow = new QHBoxLayout;
    comboRow->addWidget(new QLabel("Монитор:"));
    m_monitorCombo = new QComboBox;
    connect(m_monitorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onMonitorSelected);
    comboRow->addWidget(m_monitorCombo, 1);
    root->addLayout(comboRow);

    // панель настроек
    m_settingsGroup = new QGroupBox("Настройки монитора");
    QVBoxLayout *sg = new QVBoxLayout(m_settingsGroup);

    m_orientationLabel = new QLabel("Ориентация: —");
    m_orientationLabel->setStyleSheet("color: #aaa; font-size: 11px;");
    sg->addWidget(m_orientationLabel);

    // файл
    QHBoxLayout *fileRow = new QHBoxLayout;
    fileRow->addWidget(new QLabel("Файл:"));
    m_fileEdit  = new QLineEdit;
    m_fileEdit->setPlaceholderText("Путь к изображению или видео...");
    m_browseBtn = new QPushButton("📂 Обзор");
    connect(m_browseBtn, &QPushButton::clicked, this, &MainWindow::onBrowseFile);
    fileRow->addWidget(m_fileEdit, 1);
    fileRow->addWidget(m_browseBtn);
    sg->addLayout(fileRow);

    // аудио (только для видео)
    m_audioCheck = new QCheckBox("🔊 Включить звук");
    connect(m_audioCheck, &QCheckBox::toggled, this, &MainWindow::onAudioToggled);
    m_audioCheck->hide();
    sg->addWidget(m_audioCheck);

    QWidget *volWidget = new QWidget;
    QHBoxLayout *volRow = new QHBoxLayout(volWidget);
    volRow->setContentsMargins(0,0,0,0);
    volRow->addWidget(new QLabel("Громкость:"));
    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(50);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    m_volumeLabel = new QLabel("50%");
    m_volumeLabel->setMinimumWidth(36);
    volRow->addWidget(m_volumeSlider, 1);
    volRow->addWidget(m_volumeLabel);
    volWidget->hide();
    m_volumeSlider->setProperty("volWidget", QVariant::fromValue(volWidget));
    sg->addWidget(volWidget);

    m_bindHint = new QLabel;
    m_bindHint->setWordWrap(true);
    m_bindHint->setStyleSheet("color: #888; font-size: 11px; font-family: monospace;");
    m_bindHint->hide();
    sg->addWidget(m_bindHint);

    // заполнение
    QHBoxLayout *fillRow = new QHBoxLayout;
    fillRow->addWidget(new QLabel("Заполнение:"));
    m_fillCombo = new QComboBox;
    m_fillCombo->addItems({"Заполнить (fill)", "Вписать (contain)", "Растянуть (stretch)",
                           "По центру (center)", "Плиткой (tile)",
                           "Верх-лево", "Верх-право", "Низ-лево", "Низ-право",
                           "Верх-центр", "Низ-центр", "Центр-лево", "Центр-право"});
    connect(m_fillCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onFillModeChanged);
    fillRow->addWidget(m_fillCombo, 1);
    sg->addLayout(fillRow);

    // поворот
    QHBoxLayout *rotRow = new QHBoxLayout;
    rotRow->addWidget(new QLabel("Поворот обоев:"));
    m_rotCombo = new QComboBox;
    m_rotCombo->addItems({"Нормальный (0°)", "Книжный (90°)",
                          "Перевёрнутый (180°)", "Книжный перевёрнутый (270°)"});
    connect(m_rotCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onRotationChanged);
    rotRow->addWidget(m_rotCombo, 1);
    sg->addLayout(rotRow);

    m_applyBtn = new QPushButton("✅ Применить");
    m_applyBtn->setFixedHeight(36);
    connect(m_applyBtn, &QPushButton::clicked, this, &MainWindow::onApply);
    sg->addWidget(m_applyBtn);

    root->addWidget(m_settingsGroup);
    root->addStretch();
}

void MainWindow::loadMonitors()
{
    m_monitors = MonitorDetector::detect();
    m_monitorBar->setMonitors(m_monitors);
    m_monitorCombo->clear();
    auto &cm = ConfigManager::instance();
    for (const MonitorInfo &m : m_monitors) {
        m_monitorCombo->addItem(m.name);
        WallpaperConfig cfg = cm.getConfig(m.name);
        if (!cfg.filePath.isEmpty())
            m_monitorBar->setWallpaperPath(m.name, cfg.filePath);
    }
    if (!m_monitors.isEmpty())
        onMonitorSelected(0);
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
    if (it != m_monitors.cend()) {
        QString orient = orientationString(it->transform);
        m_orientationLabel->setText(
            QString("%1  |  %2×%3  @  %4Hz  scale %5")
            .arg(orient)
            .arg(it->width).arg(it->height)
            .arg(it->refreshRate)
            .arg(it->scale, 0, 'f', 2));
    }

    WallpaperConfig cfg = ConfigManager::instance().getConfig(monitorName);
    m_fileEdit->setText(cfg.filePath);
    m_fillCombo->setCurrentIndex(static_cast<int>(cfg.fillMode));
    m_rotCombo->setCurrentIndex(static_cast<int>(cfg.rotation));
    m_audioCheck->setChecked(cfg.audioEnabled);
    m_volumeSlider->setValue(cfg.audioVolume);
    m_volumeLabel->setText(QString("%1%").arg(cfg.audioVolume));

    bool isVid = WallpaperApplier::isVideoFile(cfg.filePath);
    m_audioCheck->setVisible(isVid);
    QWidget *vw = m_volumeSlider->property("volWidget").value<QWidget*>();
    if (vw) vw->setVisible(isVid && cfg.audioEnabled);
    if (isVid) {
        m_bindHint->setText(
            QString("💡 bind = , F9, exec, hyprwall --toggle-audio %1").arg(monitorName));
        m_bindHint->show();
    } else {
        m_bindHint->hide();
    }
}

void MainWindow::saveCurrentSettings()
{
    if (m_currentMonitor.isEmpty()) return;
    WallpaperConfig cfg = ConfigManager::instance().getConfig(m_currentMonitor);
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
    QString path = QFileDialog::getOpenFileName(this, "Выберите обои", QDir::homePath(),
        "Изображения и видео (*.jpg *.jpeg *.png *.bmp *.gif "
        "*.mp4 *.mkv *.avi *.webm *.mov);;Все файлы (*)");
    if (path.isEmpty()) return;
    m_fileEdit->setText(path);
    bool isVid = WallpaperApplier::isVideoFile(path);
    m_audioCheck->setVisible(isVid);
    QWidget *vw = m_volumeSlider->property("volWidget").value<QWidget*>();
    if (vw) vw->setVisible(isVid && m_audioCheck->isChecked());
    if (isVid) {
        m_bindHint->setText(
            QString("💡 bind = , F9, exec, hyprwall --toggle-audio %1").arg(m_currentMonitor));
        m_bindHint->show();
    } else {
        m_bindHint->hide();
    }
    m_monitorBar->setWallpaperPath(m_currentMonitor, path);
}

void MainWindow::onApply()
{
    saveCurrentSettings();
    ConfigManager::instance().save();
    WallpaperConfig cfg = ConfigManager::instance().getConfig(m_currentMonitor);
    bool ok = WallpaperApplier::apply(cfg);
    if (!ok) {
        QMessageBox::warning(this, "Ошибка",
            "Не удалось применить обои.\n\n"
            "Убедитесь что:\n"
            "• hyprpaper запущен и настроен\n"
            "• mpvpaper установлен (для видео)\n"
            "• Путь к файлу корректен");
    }
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
