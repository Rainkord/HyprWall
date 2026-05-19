#pragma once
#include <QMainWindow>
#include <QMap>
#include <QList>
#include <QTimer>
#include "Types.h"
#include "Strings.h"

class QLabel;
class QPushButton;
class QComboBox;
class QCheckBox;
class QSlider;
class QLineEdit;
class QGroupBox;
class QHBoxLayout;
class QVBoxLayout;
class QScrollArea;
class QSpinBox;
class MonitorBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onMonitorClicked(const QString &name);
    void onBrowseFile();
    void onApplyAll();
    void onFillModeChanged(int);
    void onRotationChanged(int);
    void onAudioToggled(bool checked);
    void onVolumeChanged(int val);
    void onLanguageChanged(int idx);
    void onAutostartToggle();
    // Gallery
    void onGalleryAdd();
    void onGalleryRemove(const QString &path);
    void onGalleryItemClicked(const QString &path, bool isVideo);
    // Slideshow
    void onSlideshowToggled(bool checked);
    void onSlideshowTick();

private:
    void buildUi();
    void buildGalleryPanel(QVBoxLayout *parent);
    void buildSlideshowPanel(QVBoxLayout *parent);
    void loadMonitors();
    void populateSettings(const QString &monitorName);
    void saveCurrentToPending();
    void retranslateUi();
    void refreshGallery();
    void switchToVideo(bool isVideo);
    void updateAutostartButton();
    QString bindString() const;
    QString smartBrowseDir() const;

    // --- UI elements ---
    MonitorBar    *m_monitorBar      = nullptr;
    QGroupBox     *m_settingsGroup   = nullptr;
    QLabel        *m_orientationLabel= nullptr;
    QLabel        *m_fileLabel       = nullptr;
    QLineEdit     *m_fileEdit        = nullptr;
    QPushButton   *m_browseBtn       = nullptr;
    QCheckBox     *m_audioCheck      = nullptr;
    QLabel        *m_volumeLabelW    = nullptr;
    QSlider       *m_volumeSlider    = nullptr;
    QLabel        *m_volumeLabel     = nullptr;
    QComboBox     *m_fillCombo       = nullptr;
    QComboBox     *m_rotCombo        = nullptr;
    QPushButton   *m_applyBtn        = nullptr;
    QLabel        *m_fillLabel       = nullptr;
    QLabel        *m_rotLabel        = nullptr;
    QLabel        *m_bindPrefixLabel = nullptr;
    QLabel        *m_bindHint        = nullptr;
    QWidget       *m_bindRow         = nullptr;
    QLabel        *m_autostartLabel  = nullptr;
    QPushButton   *m_autostartBtn    = nullptr;
    QLabel        *m_langLabel       = nullptr;
    QComboBox     *m_langCombo       = nullptr;

    // Gallery panel
    QGroupBox     *m_galleryGroup    = nullptr;
    QPushButton   *m_galleryAddBtn   = nullptr;
    QWidget       *m_galleryGrid     = nullptr;   // grid widget inside scroll area
    QLabel        *m_galleryEmptyLbl = nullptr;

    // Slideshow panel
    QWidget       *m_slideshowRow    = nullptr;
    QCheckBox     *m_slideshowCheck  = nullptr;
    QComboBox     *m_intervalCombo   = nullptr;
    QLabel        *m_intervalPrefixLbl = nullptr;
    QLabel        *m_intervalSuffixLbl = nullptr;

    // --- State ---
    Strings               m_s;
    bool                  m_isRU    = false;
    bool                  m_isVideo = false;
    QString               m_currentMonitor;
    QList<MonitorInfo>    m_monitors;

    // Pending changes map: monitorName -> config (not yet applied)
    QMap<QString, WallpaperConfig> m_pending;

    // Slideshow
    QTimer               *m_slideshowTimer = nullptr;
    static const int      INTERVAL_VALUES[]; // seconds per index
};
