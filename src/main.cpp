#include <iostream>
#include <string>

// #include "lib.hpp"

// Easy logging defines
#define ELPP_QT_LOGGING 1
#define ELPP_FEATURE_PERFORMANCE_TRACKING 1
#include <easyloggingpp/easylogging++.h>

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#include "gui/mainwindow.h"
#include <QSurfaceFormat>


INITIALIZE_EASYLOGGINGPP

auto main(int argc, char* argv[]) -> int
{
    el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
    LOG(INFO) << "Hello World!";
    
    // needs to be here, before we actually create the GUI!
    QSurfaceFormat form;
    form.setMajorVersion(4);
    form.setMinorVersion(1);
    form.setRenderableType(QSurfaceFormat::OpenGL);
    form.setProfile(QSurfaceFormat::CoreProfile);
//    form.setDepthBufferSize(32);
    QSurfaceFormat::setDefaultFormat(form);
    
    
    QApplication app(argc, argv);
    

    
    MainWindow* window = new MainWindow();
    
    window->show();
 
    return app.exec();
}
