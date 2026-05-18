#pragma once
#include <QMainWindow>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include <QGroupBox>
#include <QList>
#include "Types.h"
#include "MonitorDetector.h"

class MonitorBar;

struct Strings {
    QString windowTitle, langLabel, noMonitors, monitorLabel, groupTitle;
    QString orientLabel, fileLabel, browseBtn, audioCheck, volumeLabel;
    QString fillLabel, rotLabel, applyBtn;
    QString bindPrefix;
    QStringList fillModes, rotModes;
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
    s.fillModes    = {"Cover", "Contain", "Tile", "Fill"};
    s.rotModes     = {"None", "90 CW", "180", "270 CW", "Flip H", "Flip V"};
    s.bindPrefix   = "Add to hyprland.conf:";
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
    s.langLabel    = "Язык:";
    s.noMonitors   = "Мониторы не найдены";
    s.monitorLabel = "Монитор:";
    s.groupTitle   = "Настройки";
    s.fileLabel    = "Файл:";
    s.browseBtn    = "Обзор";
    s.audioCheck   = "Звук";
    s.volumeLabel  = "Громкость:";
    s.fillLabel    = "Заполнение:";
    s.rotLabel     = "Поворот:";
    s.applyBtn     = "Применить";
    s.fillModes    = {"Cover", "Contain", "Tile", "Fill"};
    s.rotModes     = {"Нет", "90 по часовой", "180", "270 по часовой", "Зеркало H", "Зеркало V"};
    s.bindPrefix   = "Добавьте в hyprland.conf:";
    s.orientLandscape    = "Альбомный";
    s.orientPortrait90   = "Книжный 90";
    s.orientLandscape180 = "Альбомный 180";
    s.orientPortrait270  = "Книжный 270";
    s.errTitle = "Ошибка";
    s.errBody  = "Не удалось применить обои.\n\nПроверьте:\n- hyprpaper установлен\n- mpvpaper установлен (видео)\n- Путь к файлу корректен";
    return s;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onMonitorSelected(int index);
    void onBrowseFile();
    void onApply();
    void onFillModeChanged(int);
    void onRotationChanged(int);
    void onAudioToggled(bool);
    void onVolumeChanged(int);
    void onLanguageChanged(int);

private:
    void buildUi();
    void loadMonitors();
    void populateSettings(const QString &monitorName);
    void saveCurrentSettings();
    void retranslateUi();
    QString bindString() const;

    Strings  m_s;
    bool     m_isRU = false;

    MonitorBar  *m_monitorBar       = nullptr;
    QComboBox   *m_monitorCombo     = nullptr;
    QGroupBox   *m_settingsGroup    = nullptr;
    QLabel      *m_orientationLabel = nullptr;
    QLineEdit   *m_fileEdit         = nullptr;
    QPushButton *m_browseBtn        = nullptr;
    QCheckBox   *m_audioCheck       = nullptr;
    QSlider     *m_volumeSlider     = nullptr;
    QLabel      *m_volumeLabel      = nullptr;
    QComboBox   *m_fillCombo        = nullptr;
    QComboBox   *m_rotCombo         = nullptr;
    QPushButton *m_applyBtn         = nullptr;
    QLabel      *m_bindHint         = nullptr;
    QWidget     *m_bindRow          = nullptr;

    // retranslate targets
    QLabel    *m_langLabel      = nullptr;
    QComboBox *m_langCombo      = nullptr;
    QLabel    *m_monitorLabel   = nullptr;
    QLabel    *m_fileLabel      = nullptr;
    QLabel    *m_volumeLabelW   = nullptr;
    QLabel    *m_fillLabel      = nullptr;
    QLabel    *m_rotLabel       = nullptr;
    QLabel    *m_bindPrefixLabel= nullptr;

    QList<MonitorInfo> m_monitors;
    QString            m_currentMonitor;
};
