#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "MainWindow.h"
#include "WallpaperApplier.h"
#include "ConfigManager.h"
#include "MonitorDetector.h"
#include "ServiceManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("HyprWall");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Rainkord");

    QCommandLineParser parser;
    parser.setApplicationDescription("Менеджер обоев для Hyprland");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption toggleAudio(
        "toggle-audio",
        "Переключить звук видео-обоев на указанном мониторе",
        "monitor");
    parser.addOption(toggleAudio);

    QCommandLineOption daemonMode(
        "daemon",
        "Восстановить обои (запускается systemd-сервисом)");
    parser.addOption(daemonMode);

    parser.process(app);

    if (parser.isSet(toggleAudio)) {
        WallpaperApplier::toggleAudio(parser.value(toggleAudio));
        return 0;
    }

    if (parser.isSet(daemonMode)) {
        ServiceManager::applyAll(MonitorDetector::detect());
        return 0;
    }

    MainWindow w;
    w.show();
    return app.exec();
}
