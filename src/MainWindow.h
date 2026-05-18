#pragma once
#include "Types.h"
#include "MonitorDetector.h"
#include <QMainWindow>
#include <QList>
#include <QLabel>
#include <QComboBox>
#include <QSlider>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>

class MonitorBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onMonitorSelected(int index);
    void onBrowseFile();
    void onApply();
    void onFillModeChanged(int index);
    void onRotationChanged(int index);
    void onAudioToggled(bool checked);
    void onVolumeChanged(int value);

private:
    void buildUi();
    void loadMonitors();
    void populateSettings(const QString &monitorName);
    void saveCurrentSettings();

    QList<MonitorInfo>  m_monitors;
    QString             m_currentMonitor;

    MonitorBar         *m_monitorBar   = nullptr;
    QComboBox          *m_monitorCombo = nullptr;
    QGroupBox          *m_settingsGroup = nullptr;
    QLabel             *m_orientationLabel = nullptr;
    QLineEdit          *m_fileEdit    = nullptr;
    QPushButton        *m_browseBtn   = nullptr;
    QComboBox          *m_fillCombo   = nullptr;
    QComboBox          *m_rotCombo    = nullptr;
    QCheckBox          *m_audioCheck  = nullptr;
    QSlider            *m_volumeSlider = nullptr;
    QLabel             *m_volumeLabel  = nullptr;
    QLabel             *m_bindHint    = nullptr;
    QPushButton        *m_applyBtn    = nullptr;
};
