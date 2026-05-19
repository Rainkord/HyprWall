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
#include <QWidget>
#include <QScrollArea>
#include <QGridLayout>
#include <QVBoxLayout>

// MonitorBar is defined in MainWindow.cpp (inline, no separate header)
class MonitorBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    // Called by gallery thumbnail widgets
    void onGalleryRemove(const QString &path);
    void onGalleryItemClicked(const QString &path, bool isVideo);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private slots:
    void onMonitorClicked(const QString &name);
    void onBrowseFile();
    void onApplyAll();
    void onGalleryAdd();
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
    void buildGalleryPanel(QVBoxLayout *parent);
    void buildSlideshowPanel(QVBoxLayout *parent);
    void loadMonitors();
    void populateSettings(const QString &monitorName);
    void saveCurrentToPending();
    void refreshGallery();
    void switchToVideo(bool isVideo);
    void retranslateUi();
    void updateAutostartButton();
    QString bindString() const;
    QString smartBrowseDir() const;

    static const int INTERVAL_VALUES[6];

    MonitorBar      *m_monitorBar          = nullptr;
    QGroupBox       *m_settingsGroup       = nullptr;
    QLabel          *m_orientationLabel    = nullptr;
    QLabel          *m_fileLabel           = nullptr;
    QLineEdit       *m_fileEdit            = nullptr;
    QPushButton     *m_browseBtn           = nullptr;
    QCheckBox       *m_audioCheck          = nullptr;
    QLabel          *m_volumeLabelW        = nullptr;
    QSlider         *m_volumeSlider        = nullptr;
    QLabel          *m_volumeLabel         = nullptr;
    QWidget         *m_bindRow             = nullptr;
    QLabel          *m_bindPrefixLabel     = nullptr;
    QLabel          *m_bindHint            = nullptr;
    QLabel          *m_fillLabel           = nullptr;
    QComboBox       *m_fillCombo           = nullptr;
    QLabel          *m_rotLabel            = nullptr;
    QComboBox       *m_rotCombo            = nullptr;
    QPushButton     *m_applyBtn            = nullptr;
    QLabel          *m_langLabel           = nullptr;
    QComboBox       *m_langCombo           = nullptr;
    QLabel          *m_autostartLabel      = nullptr;
    QPushButton     *m_autostartBtn        = nullptr;

    // Gallery
    QGroupBox       *m_galleryGroup        = nullptr;
    QPushButton     *m_galleryAddBtn       = nullptr;
    QWidget         *m_galleryGrid         = nullptr;
    QLabel          *m_galleryEmptyLbl     = nullptr;

    // Slideshow
    QWidget         *m_slideshowRow        = nullptr;
    QCheckBox       *m_slideshowCheck      = nullptr;
    QLabel          *m_intervalPrefixLbl   = nullptr;
    QComboBox       *m_intervalCombo       = nullptr;
    QLabel          *m_intervalSuffixLbl   = nullptr;
    QTimer          *m_slideshowTimer      = nullptr;

    QList<MonitorInfo>             m_monitors;
    QString                        m_currentMonitor;
    QMap<QString, WallpaperConfig> m_pending;
    bool                           m_isVideo = false;
    bool                           m_isRU    = false;
    Strings                        m_s;
};
