#pragma once
#include "Types.h"
#include "MonitorDetector.h"
#include "Strings.h"

#include <QMainWindow>
#include <QMap>
#include <QProcess>

class QLabel;
class QLineEdit;
class QPushButton;
class QComboBox;
class QCheckBox;
class QSlider;
class QGroupBox;
class QRadioButton;
class QButtonGroup;
class QStackedWidget;
class MonitorBar;
class QTimer;

struct SlideState {
    QStringList files;
    int index = 0;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static const int INTERVAL_SECS[4];

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;

private slots:
    void onMonitorSelected(int index);
    void onBrowseFile();
    void onBrowseFolder();
    void onApply();
    void onFillModeChanged(int);
    void onRotationChanged(int);
    void onAudioToggled(bool);
    void onVolumeChanged(int);
    void onLanguageChanged(int);
    void onAutostartToggle();
    void onModeChanged(int);
    void onSlideshowTick();   // kept for compat, no-op

private:
    void buildUi();
    void loadMonitors();
    void populateSettings(const QString &monitorName);
    void saveCurrentSettings();
    void switchToVideo(bool isVideo);
    void retranslateUi();
    void updateModeStack(int idx);
    QString bindString() const;
    QString smartBrowseDir() const;
    bool isAutostartEnabled() const;
    void updateAutostartButton();

    // Slideshow via external script
    void startSlideshowScript(const QString &monitor, const QString &folder, int secs);
    void stopSlideshowScript(const QString &monitor);
    void stopAllSlideshowScripts();

    // Compat stubs
    void startSlideshowTimer();
    void stopSlideshowTimer();
    void applyNextSlide();

    // UI
    MonitorBar      *m_monitorBar      = nullptr;
    QLabel          *m_monitorLabel    = nullptr;
    QComboBox       *m_monitorCombo    = nullptr;
    QGroupBox       *m_settingsGroup   = nullptr;
    QLabel          *m_orientationLabel= nullptr;
    QRadioButton    *m_radioStatic     = nullptr;
    QRadioButton    *m_radioSlideshow  = nullptr;
    QButtonGroup    *m_modeGroup       = nullptr;
    QStackedWidget  *m_modeStack       = nullptr;
    QLabel          *m_fileLabel       = nullptr;
    QLineEdit       *m_fileEdit        = nullptr;
    QPushButton     *m_browseBtn       = nullptr;
    QCheckBox       *m_audioCheck      = nullptr;
    QLabel          *m_volumeLabelW    = nullptr;
    QSlider         *m_volumeSlider    = nullptr;
    QLabel          *m_volumeLabel     = nullptr;
    QWidget         *m_bindRow         = nullptr;
    QLabel          *m_bindPrefixLabel = nullptr;
    QLabel          *m_bindHint        = nullptr;
    QLabel          *m_folderLabel     = nullptr;
    QLineEdit       *m_folderEdit      = nullptr;
    QPushButton     *m_browseFolderBtn = nullptr;
    QLabel          *m_intervalLabel   = nullptr;
    QComboBox       *m_intervalCombo   = nullptr;
    QLabel          *m_fillLabel       = nullptr;
    QComboBox       *m_fillCombo       = nullptr;
    QLabel          *m_rotLabel        = nullptr;
    QComboBox       *m_rotCombo        = nullptr;
    QPushButton     *m_applyBtn        = nullptr;
    QLabel          *m_langLabel       = nullptr;
    QComboBox       *m_langCombo       = nullptr;
    QLabel          *m_autostartLabel  = nullptr;
    QPushButton     *m_autostartBtn    = nullptr;

    // State
    QList<MonitorInfo>  m_monitors;
    QString             m_currentMonitor;
    QPoint              m_dragPos;
    bool                m_isVideo = false;
    bool                m_isRU    = false;
    Strings             m_s;
    QMap<QString,SlideState>  m_slideStates;   // kept for compat
    QMap<QString,QProcess*>   m_slideshowProcs;
    QTimer             *m_slideshowTimer = nullptr; // kept for compat
};
