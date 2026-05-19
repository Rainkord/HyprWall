#pragma once
#include <QMainWindow>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QTimer>
#include <QList>
#include <QPoint>
#include <QStringList>
#include "Types.h"
#include "MonitorDetector.h"

class MonitorBar;

struct Strings {
    QString windowTitle, langLabel, noMonitors, monitorLabel, groupTitle;
    QString fileLabel, browseBtn, audioCheck, volumeLabel;
    QString fillLabel, rotLabel, applyBtn, bindPrefix;
    QString autostartLabel, autostartEnable, autostartDisable;
    QString modeStatic, modeSlideshow;
    QString folderLabel, browseFolderBtn;
    QString intervalLabel;
    QStringList intervalLabels;   // 10 min, 20 min ...
    QStringList imgFillModes, imgRotModes, vidFillModes, vidRotModes;
    QString orientLandscape, orientPortrait90, orientLandscape180, orientPortrait270;
    QString errTitle, errBody;
};

static Strings stringsEN() {
    Strings s;
    s.windowTitle  = "HyprWall";
    s.langLabel    = "Language:";
    s.noMonitors   = "No monitors found";
    s.monitorLabel = "Monitor:";
    s.groupTitle   = "Monitor settings";
    s.fileLabel    = "File:";
    s.browseBtn    = "Browse";
    s.audioCheck   = "Audio";
    s.volumeLabel  = "Volume:";
    s.fillLabel    = "Fill:";
    s.rotLabel     = "Rotation:";
    s.applyBtn     = "Apply";
    s.bindPrefix   = "Add to hyprland.conf:";
    s.autostartLabel   = "Autostart:";
    s.autostartEnable  = "Enable";
    s.autostartDisable = "Disable";
    s.modeStatic    = "Single file";
    s.modeSlideshow = "Slideshow";
    s.folderLabel      = "Folder:";
    s.browseFolderBtn  = "Browse";
    s.intervalLabel    = "Change every:";
    s.intervalLabels   = {"10 min", "20 min", "30 min", "1 hour"};
    s.imgFillModes = {"Cover", "Contain"};
    s.imgRotModes  = {"0\u00b0", "90\u00b0", "180\u00b0", "270\u00b0", "Flip H", "Flip V"};
    s.vidFillModes = {"Cover", "Contain", "Fill"};
    s.vidRotModes  = {"0\u00b0", "90\u00b0", "180\u00b0", "270\u00b0", "Flip H", "Flip V"};
    s.orientLandscape    = "Landscape";
    s.orientPortrait90   = "Portrait 90";
    s.orientLandscape180 = "Landscape 180";
    s.orientPortrait270  = "Portrait 270";
    s.errTitle = "Error";
    s.errBody  = "Failed to apply wallpaper.\n\nCheck:\n- hyprpaper installed\n- mpvpaper installed (video)\n- File path is correct";
    return s;
}

static Strings stringsRU() {
    Strings s;
    s.windowTitle  = "HyprWall";
    s.langLabel    = "\u042f\u0437\u044b\u043a:";
    s.noMonitors   = "\u041c\u043e\u043d\u0438\u0442\u043e\u0440\u044b \u043d\u0435 \u043d\u0430\u0439\u0434\u0435\u043d\u044b";
    s.monitorLabel = "\u041c\u043e\u043d\u0438\u0442\u043e\u0440:";
    s.groupTitle   = "\u041d\u0430\u0441\u0442\u0440\u043e\u0439\u043a\u0438";
    s.fileLabel    = "\u0424\u0430\u0439\u043b:";
    s.browseBtn    = "\u041e\u0431\u0437\u043e\u0440";
    s.audioCheck   = "\u0417\u0432\u0443\u043a";
    s.volumeLabel  = "\u0413\u0440\u043e\u043c\u043a\u043e\u0441\u0442\u044c:";
    s.fillLabel    = "\u0417\u0430\u043f\u043e\u043b\u043d\u0435\u043d\u0438\u0435:";
    s.rotLabel     = "\u041f\u043e\u0432\u043e\u0440\u043e\u0442:";
    s.applyBtn     = "\u041f\u0440\u0438\u043c\u0435\u043d\u0438\u0442\u044c";
    s.bindPrefix   = "\u0414\u043e\u0431\u0430\u0432\u044c\u0442\u0435 \u0432 hyprland.conf:";
    s.autostartLabel   = "\u0410\u0432\u0442\u043e\u0437\u0430\u043f\u0443\u0441\u043a:";
    s.autostartEnable  = "\u0423\u0441\u0442\u0430\u043d\u043e\u0432\u0438\u0442\u044c";
    s.autostartDisable = "\u0421\u043d\u044f\u0442\u044c";
    s.modeStatic    = "\u041e\u0434\u0438\u043d \u0444\u0430\u0439\u043b";
    s.modeSlideshow = "\u0421\u043b\u0430\u0439\u0434\u0448\u043e\u0443";
    s.folderLabel     = "\u041f\u0430\u043f\u043a\u0430:";
    s.browseFolderBtn = "\u041e\u0431\u0437\u043e\u0440";
    s.intervalLabel   = "\u041c\u0435\u043d\u044f\u0442\u044c \u043a\u0430\u0436\u0434\u044b\u0435:";
    s.intervalLabels  = {"10 \u043c\u0438\u043d", "20 \u043c\u0438\u043d", "30 \u043c\u0438\u043d", "1 \u0447\u0430\u0441"};
    s.imgFillModes = {"\u041f\u043e\u043a\u0440\u044b\u0442\u0438\u0435", "\u0412\u043f\u0438\u0441\u0430\u0442\u044c"};
    s.imgRotModes  = {"0\u00b0", "90\u00b0", "180\u00b0", "270\u00b0", "\u0417\u0435\u0440\u043a\u0430\u043b\u043e H", "\u0417\u0435\u0440\u043a\u0430\u043b\u043e V"};
    s.vidFillModes = {"\u041f\u043e\u043a\u0440\u044b\u0442\u0438\u0435", "\u0412\u043f\u0438\u0441\u0430\u0442\u044c", "\u0420\u0430\u0441\u0442\u044f\u043d\u0443\u0442\u044c"};
    s.vidRotModes  = {"0\u00b0", "90\u00b0", "180\u00b0", "270\u00b0", "\u0417\u0435\u0440\u043a\u0430\u043b\u043e H", "\u0417\u0435\u0440\u043a\u0430\u043b\u043e V"};
    s.orientLandscape    = "\u0410\u043b\u044c\u0431\u043e\u043c\u043d\u044b\u0439";
    s.orientPortrait90   = "\u041a\u043d\u0438\u0436\u043d\u044b\u0439 90";
    s.orientLandscape180 = "\u0410\u043b\u044c\u0431\u043e\u043c\u043d\u044b\u0439 180";
    s.orientPortrait270  = "\u041a\u043d\u0438\u0436\u043d\u044b\u0439 270";
    s.errTitle = "\u041e\u0448\u0438\u0431\u043a\u0430";
    s.errBody  = "\u041d\u0435 \u0443\u0434\u0430\u043b\u043e\u0441\u044c \u043f\u0440\u0438\u043c\u0435\u043d\u0438\u0442\u044c \u043e\u0431\u043e\u0438.\n\n\u041f\u0440\u043e\u0432\u0435\u0440\u044c\u0442\u0435:\n- hyprpaper \u0443\u0441\u0442\u0430\u043d\u043e\u0432\u043b\u0435\u043d\n- mpvpaper \u0443\u0441\u0442\u0430\u043d\u043e\u0432\u043b\u0435\u043d (\u0432\u0438\u0434\u0435\u043e)\n- \u041f\u0443\u0442\u044c \u043a \u0444\u0430\u0439\u043b\u0443 \u043a\u043e\u0440\u0440\u0435\u043a\u0442\u0435\u043d";
    return s;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

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
    void onSlideshowTick();

private:
    void buildUi();
    void loadMonitors();
    void populateSettings(const QString &monitorName);
    void saveCurrentSettings();
    void retranslateUi();
    void switchToVideo(bool isVideo);
    void updateAutostartButton();
    void updateModeStack(int modeIdx);
    void startSlideshowTimer();
    void stopSlideshowTimer();
    void applyNextSlide();
    QString smartBrowseDir() const;
    QString bindString() const;
    bool isAutostartEnabled() const;

    static const int INTERVAL_SECS[4];

    Strings  m_s;
    bool     m_isRU    = false;
    bool     m_isVideo = false;
    QPoint   m_dragPos;

    // per-monitor slideshow state
    struct SlideState {
        QStringList files;
        int         index = 0;
    };
    QMap<QString, SlideState> m_slideStates;
    QTimer *m_slideshowTimer = nullptr;

    MonitorBar   *m_monitorBar       = nullptr;
    QComboBox    *m_monitorCombo     = nullptr;
    QGroupBox    *m_settingsGroup    = nullptr;
    QLabel       *m_orientationLabel = nullptr;

    // mode selector
    QRadioButton *m_radioStatic      = nullptr;
    QRadioButton *m_radioSlideshow   = nullptr;
    QButtonGroup *m_modeGroup        = nullptr;
    QStackedWidget *m_modeStack      = nullptr;

    // Static page
    QLineEdit   *m_fileEdit          = nullptr;
    QPushButton *m_browseBtn         = nullptr;
    QCheckBox   *m_audioCheck        = nullptr;
    QSlider     *m_volumeSlider      = nullptr;
    QLabel      *m_volumeLabel       = nullptr;
    QWidget     *m_bindRow           = nullptr;
    QLabel      *m_bindHint          = nullptr;

    // Slideshow page
    QLineEdit   *m_folderEdit        = nullptr;
    QPushButton *m_browseFolderBtn   = nullptr;
    QComboBox   *m_intervalCombo     = nullptr;

    // Shared
    QComboBox   *m_fillCombo         = nullptr;
    QComboBox   *m_rotCombo          = nullptr;
    QPushButton *m_applyBtn          = nullptr;

    QLabel    *m_langLabel           = nullptr;
    QComboBox *m_langCombo           = nullptr;
    QLabel    *m_monitorLabel        = nullptr;
    QLabel    *m_fileLabel           = nullptr;
    QLabel    *m_volumeLabelW        = nullptr;
    QLabel    *m_fillLabel           = nullptr;
    QLabel    *m_rotLabel            = nullptr;
    QLabel    *m_bindPrefixLabel     = nullptr;
    QLabel    *m_autostartLabel      = nullptr;
    QPushButton *m_autostartBtn      = nullptr;
    QLabel    *m_folderLabel         = nullptr;
    QLabel    *m_intervalLabel       = nullptr;

    QList<MonitorInfo> m_monitors;
    QString            m_currentMonitor;
};
