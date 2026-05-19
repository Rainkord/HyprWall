#pragma once
#include <QMainWindow>
#include <QList>
#include <QMap>
#include <QPoint>
#include "MonitorDetector.h"
#include "Types.h"
#include "Strings.h"

class QLabel;
class QPushButton;
class QComboBox;
class QCheckBox;
class QSlider;
class QLineEdit;
class QGroupBox;
class QVBoxLayout;
class QTimer;
class MonitorBar;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    // Called by gallery thumb close-button lambda
    void onGalleryRemove(const QString &path);

    static const int INTERVAL_VALUES[6];

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    bool eventFilter(QObject *, QEvent *) override;

private slots:
    void onAutostartToggle();
    void onMonitorClicked(const QString &name);
    void onApplyAll();
    void onGalleryAdd();
    void onGalleryItemClicked(const QString &path, bool isVideo);
    void onSlideshowToggled(bool checked);
    void onSlideshowTick();
    void onFillModeChanged(int);
    void onRotationChanged(int);
    void onAudioToggled(bool checked);
    void onVolumeChanged(int val);
    void onLanguageChanged(int idx);

private:
    void buildUi();
    void buildGalleryPanel(QVBoxLayout *parent);
    void refreshGallery();
    void loadMonitors();
    void populateSettings(const QString &monitorName);
    void saveCurrentToPending();
    void switchToVideo(bool isVideo);
    void retranslateUi();
    void updateAutostartButton();
    void updateSlideshowDependentWidgets(bool ssOn);
    QString bindString() const;
    QString smartBrowseDir() const;
    bool isAutostartEnabled() const;

    // UI
    MonitorBar   *m_monitorBar    = nullptr;
    QGroupBox    *m_settingsGroup = nullptr;
    QLabel       *m_orientationLabel = nullptr;

    // slideshow
    QCheckBox    *m_slideshowCheck     = nullptr;
    QWidget      *m_timerRow           = nullptr;
    QWidget      *m_mediaModeRow       = nullptr;
    QLabel       *m_intervalPrefixLbl  = nullptr;
    QLabel       *m_intervalSuffixLbl  = nullptr;
    QComboBox    *m_intervalCombo      = nullptr;
    QComboBox    *m_mediaModeCombo     = nullptr;
    QTimer       *m_slideshowTimer     = nullptr;

    // gallery
    QGroupBox    *m_galleryGroup    = nullptr;
    QPushButton  *m_galleryAddBtn   = nullptr;
    QWidget      *m_galleryGrid     = nullptr;
    QLabel       *m_galleryEmptyLbl = nullptr;

    // audio
    QWidget      *m_audioRow    = nullptr;
    QCheckBox    *m_audioCheck  = nullptr;
    QWidget      *m_volumeRow   = nullptr;
    QLabel       *m_volumeLabelW = nullptr;
    QSlider      *m_volumeSlider = nullptr;
    QLabel       *m_volumeLabel  = nullptr;

    // fill / rotation
    QWidget      *m_fillRow   = nullptr;
    QLabel       *m_fillLabel = nullptr;
    QComboBox    *m_fillCombo = nullptr;
    QWidget      *m_rotRow    = nullptr;
    QLabel       *m_rotLabel  = nullptr;
    QComboBox    *m_rotCombo  = nullptr;

    // bind hint
    QWidget      *m_bindRow         = nullptr;
    QLabel       *m_bindPrefixLabel = nullptr;
    QLabel       *m_bindHint        = nullptr;

    // apply
    QPushButton  *m_applyBtn = nullptr;

    // top bar
    QLabel       *m_langLabel      = nullptr;
    QComboBox    *m_langCombo      = nullptr;
    QLabel       *m_autostartLabel = nullptr;
    QPushButton  *m_autostartBtn   = nullptr;

    // state
    QList<MonitorInfo>        m_monitors;
    QString                   m_currentMonitor;
    QMap<QString,WallpaperConfig> m_pending;
    bool                      m_isVideo = false;
    bool                      m_isRU    = false;
    Strings                   m_s;
    QPoint                    m_dragPos;
};
