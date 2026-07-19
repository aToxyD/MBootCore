#include <QApplication>
#include <QCommandLineParser>
#include <QStyleFactory>

#include "gui/framework/ApplicationController.hpp"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("MBoot Studio");
    app.setApplicationVersion(MBOOTCORE_VERSION);
    app.setOrganizationName("MBootCore");
    app.setOrganizationDomain("mbootcore.io");

    QCommandLineParser parser;
    parser.setApplicationDescription("Professional BootROM Protocol GUI Platform");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);

    app.setStyle(QStyleFactory::create("Fusion"));

    ApplicationController controller;
    if (!controller.initialize())
        return 1;

    return controller.run();
}
