#pragma once
#include <QMainWindow>
#include <QPoint>
#include <QMap>
#include <QTimer>
#include "Types.h"
#include "Strings.h"

class QLabel;
class QPushButton;
class QComboBox;
class QCheckBox;
class QSlider;
class QGroupBox;
class QVBoxLayout;
class QWidget;
class MonitorBar;
class ToggleSwitch;

struct MonitorSlideshowState {
    bool    enabled     = false;
    int     intervalSecs= 300;
    int     mediaMode   = 2; // 0=photos,1=videos,2=both
    QTimer *timer       = nullptr;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    bool eventFilter(QObject *obj, QEvent *ev) override;

    void onGalleryRemove(const QString &path);
    void onGalleryItemClicked(const QString &path, bool isVideo);

    static const int INTERVAL_VALUES[6];

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;

private slots:
    void onMonitorClicked(const QString &name);
    void onAutostartToggle();
    void onLanguageChanged(int idx);
    void onApplyAll();
    void onFillModeChanged(int);
    void onRotationChanged(int);
    void onAudioToggled(bool);
    void onVolumeChanged(int);
    void onSlideshowToggled(bool);
    void onGalleryAdd();

private:
    void buildUi();
    void buildGalleryPanel(QVBoxLayout *parent);
    void loadMonitors();
    void populateSettings(const QString &monitorName);
    void saveCurrentToPending();
    void retranslateUi();
    void updateAutostartSwitch();
    void switchToVideo(bool isVideo);
    void updateSlideshowDependentWidgets(bool ssOn);
    void refreshGallery();
    QString bindString() const;
    QString smartBrowseDir() const;

    // Per-monitor slideshow
    MonitorSlideshowState &slideshowState(const QString &monitor);
    void startSlideshowForMonitor(const QString &monitor);
    void stopSlideshowForMonitor(const QString &monitor);
    void tickMonitor(const QString &monitor);

    // State
    QList<MonitorInfo>  m_monitors;
    QString             m_currentMonitor;
    QPoint              m_dragPos;
    Strings             m_s;
    bool                m_isRU   = false;
    bool                m_isVideo= false;
    bool                m_updatingControls = false;
    QMap<QString, WallpaperConfig>        m_pending;
    QMap<QString, MonitorSlideshowState>  m_ssState;

    // UI
    MonitorBar   *m_monitorBar       = nullptr;
    QGroupBox    *m_settingsGroup    = nullptr;
    QLabel       *m_orientationLabel = nullptr;

    // Autostart toggle-switch
    QLabel       *m_autostartLabel   = nullptr;
    ToggleSwitch *m_autostartSwitch  = nullptr;

    QLabel       *m_langLabel        = nullptr;
    QComboBox    *m_langCombo        = nullptr;

    // Slideshow controls
    QCheckBox    *m_slideshowCheck   = nullptr;
    QWidget      *m_timerRow         = nullptr;
    QLabel       *m_intervalPrefixLbl= nullptr;
    QComboBox    *m_intervalCombo    = nullptr;
    QLabel       *m_intervalSuffixLbl= nullptr;
    QWidget      *m_mediaModeRow     = nullptr;
    QLabel       *m_mediaModeLabel   = nullptr;
    QComboBox    *m_mediaModeCombo   = nullptr;

    // Gallery
    QGroupBox    *m_galleryGroup     = nullptr;
    QPushButton  *m_galleryAddBtn    = nullptr;
    QWidget      *m_galleryGrid      = nullptr;
    QLabel       *m_galleryEmptyLbl  = nullptr;

    // Audio / video
    QWidget      *m_audioRow         = nullptr;
    QCheckBox    *m_audioCheck       = nullptr;
    QWidget      *m_volumeRow        = nullptr;
    QLabel       *m_volumeLabelW     = nullptr;
    QSlider      *m_volumeSlider     = nullptr;
    QLabel       *m_volumeLabel      = nullptr;

    // Fill / rotation
    QWidget      *m_fillRow          = nullptr;
    QLabel       *m_fillLabel        = nullptr;
    QComboBox    *m_fillCombo        = nullptr;
    QWidget      *m_rotRow           = nullptr;
    QLabel       *m_rotLabel         = nullptr;
    QComboBox    *m_rotCombo         = nullptr;

    // Bind hint
    QWidget      *m_bindRow          = nullptr;
    QLabel       *m_bindPrefixLabel  = nullptr;
    QLabel       *m_bindHint         = nullptr;

    QPushButton  *m_applyBtn         = nullptr;
};
