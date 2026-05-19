#pragma once
#include "Types.h"
#include "MonitorDetector.h"
#include "Strings.h"

#include <QMainWindow>
#include <QMap>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QGroupBox>
#include <QTimer>
#include <QSpinBox>
#include <QScrollArea>
#include <QGridLayout>

class MonitorBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;

private slots:
    void onMonitorClicked(const QString &name);
    void onBrowseFile();
    void onApply();
    void onAddToGallery();
    void onSlideshowToggled(bool checked);
    void onSlideshowTick();
    void onFillModeChanged(int);
    void onRotationChanged(int);
    void onAudioToggled(bool);
    void onVolumeChanged(int);
    void onLanguageChanged(int);
    void onAutostartToggle();

private:
    void buildUi();
    void loadMonitors();
    void populateSettings(const QString &monitorName);
    void saveCurrentToPending();
    void refreshGallery();
    void switchToVideo(bool isVideo);
    void retranslateUi();
    QString bindString() const;
    QString smartBrowseDir() const;
    bool isAutostartEnabled() const;
    void updateAutostartButton();

    MonitorBar      *m_monitorBar       = nullptr;
    QGroupBox       *m_settingsGroup    = nullptr;
    QLabel          *m_orientationLabel = nullptr;
    QLabel          *m_fileLabel        = nullptr;
    QLineEdit       *m_fileEdit         = nullptr;
    QPushButton     *m_browseBtn        = nullptr;
    QCheckBox       *m_audioCheck       = nullptr;
    QLabel          *m_volumeLabelW     = nullptr;
    QSlider         *m_volumeSlider     = nullptr;
    QLabel          *m_volumeLabel      = nullptr;
    QWidget         *m_bindRow          = nullptr;
    QLabel          *m_bindPrefixLabel  = nullptr;
    QLabel          *m_bindHint         = nullptr;
    QLabel          *m_fillLabel        = nullptr;
    QComboBox       *m_fillCombo        = nullptr;
    QLabel          *m_rotLabel         = nullptr;
    QComboBox       *m_rotCombo         = nullptr;
    QPushButton     *m_applyBtn         = nullptr;
    QLabel          *m_langLabel        = nullptr;
    QComboBox       *m_langCombo        = nullptr;
    QLabel          *m_autostartLabel   = nullptr;
    QPushButton     *m_autostartBtn     = nullptr;

    QGroupBox       *m_galleryGroup     = nullptr;
    QWidget         *m_galleryContents  = nullptr;
    QGridLayout     *m_galleryGrid      = nullptr;

    QGroupBox       *m_slideshowGroup   = nullptr;
    QCheckBox       *m_slideshowCheck   = nullptr;
    QSpinBox        *m_intervalSpin     = nullptr;
    QTimer          *m_slideshowTimer   = nullptr;

    QList<MonitorInfo>              m_monitors;
    QString                         m_currentMonitor;
    QMap<QString, WallpaperConfig>  m_pendingConfigs;
    QPoint                          m_dragPos;
    bool                            m_isVideo = false;
    bool                            m_isRU    = false;
    Strings                         m_s;
};
