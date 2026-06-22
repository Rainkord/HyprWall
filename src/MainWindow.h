#pragma once
#include <QMainWindow>
#include <QPoint>
#include <QMap>
#include <QTimer>
#include <QPixmap>
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
class QListWidget;
class MonitorBar;
class ToggleSwitch;
class QFileSystemWatcher;

struct MonitorSlideshowState {
    bool    enabled      = false;
    int     intervalSecs = 300;
    int     mediaMode    = 2; // 0=photos, 1=videos, 2=both
    QTimer *timer        = nullptr;
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
    void resizeEvent(QResizeEvent *ev) override;

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
    void applyAndSaveCurrent();
    void retranslateUi();
    void updateAutostartSwitch();
    void switchToVideo(bool isVideo);
    void updateSlideshowDependentWidgets(bool ssOn);
    void refreshGallery();
    void recalcGalleryLayout();
    void loadThumbAsync(const QString &path, int generation);
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
    bool                m_isRU             = false;
    bool                m_isVideo          = false;
    bool                m_updatingControls = false;
    QMap<QString, WallpaperConfig>       m_pending;
    QMap<QString, MonitorSlideshowState> m_ssState;

    // Gallery adaptive layout
    int m_thumbW = 120;
    int m_thumbH = 68;
    int m_gridW  = 126;
    int m_gridH  = 88;

    // Thumbnail cache: path -> scaled QPixmap (THUMB_W x THUMB_H)
    QMap<QString, QPixmap> m_thumbCache;
    // Incremented on every refreshGallery() so stale async results are ignored
    int m_thumbGeneration = 0;

    // Gallery filesystem watcher — triggers refreshGallery on changes
    QFileSystemWatcher *m_galleryWatcher = nullptr;

    // UI
    MonitorBar   *m_monitorBar       = nullptr;
    QGroupBox    *m_settingsGroup    = nullptr;
    QLabel       *m_orientationLabel = nullptr;

    // Autostart toggle-switch
    QLabel       *m_autostartLabel   = nullptr;
    ToggleSwitch *m_autostartSwitch  = nullptr;

    QLabel       *m_langLabel        = nullptr;
    QComboBox    *m_langCombo        = nullptr;

    // Slideshow toggle-switch
    QLabel       *m_slideshowLabel   = nullptr;
    ToggleSwitch *m_slideshowSwitch  = nullptr;

    // Slideshow sub-controls
    QWidget      *m_timerRow          = nullptr;
    QLabel       *m_intervalPrefixLbl = nullptr;
    QComboBox    *m_intervalCombo     = nullptr;
    QLabel       *m_intervalSuffixLbl = nullptr;
    QWidget      *m_mediaModeRow      = nullptr;
    QLabel       *m_mediaModeLabel    = nullptr;
    QComboBox    *m_mediaModeCombo    = nullptr;

    // Gallery — QListWidget in IconMode (no manual grid math)
    QGroupBox    *m_galleryGroup     = nullptr;
    QPushButton  *m_galleryAddBtn    = nullptr;
    QListWidget  *m_galleryList      = nullptr;
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
};
