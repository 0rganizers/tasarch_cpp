#include <iostream>
#include <string>

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#include "gui/MainWindow.h"
#include <QSurfaceFormat>

#include "log/logging.h"

auto main(int argc, char* argv[]) -> int
{
    tasarch::log::setup_logging();
    
    auto logger = tasarch::log::get("tasarch");
    logger->info("Initializing QT application...");
    
    // needs to be here, before we actually create the GUI!
    QSurfaceFormat form;
    form.setMajorVersion(4);
    form.setMinorVersion(1);
    form.setRenderableType(QSurfaceFormat::OpenGL);
    form.setProfile(QSurfaceFormat::CoreProfile);
//    form.setDepthBufferSize(32);
    QSurfaceFormat::setDefaultFormat(form);
    
    
    QApplication app(argc, argv);
    
    auto window = new tasarch::gui::MainWindow();
    
    window->show();
 
    return app.exec();
}
