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

#include "gui/MainWindow.h"
#include <QSurfaceFormat>

#include "log/logging.h"

INITIALIZE_EASYLOGGINGPP

class Testing : public tasarch::log::WithLogger {
public:
    Testing() : tasarch::log::WithLogger("testing") {}

    void test() {
        CINFO("info message here!");
    }
};

auto main(int argc, char* argv[]) -> int
{
    tasarch::log::setup_logging();
    
    auto logger = tasarch::log::get("tasarch");
    INFO(logger, "info test");
    logger->info("Info member method: {}", "asdf");
    logger->error("error member method!");
    logger->error("error member method param: {}", "asdf");
    
    auto testing = new Testing();
    testing->test();
    
    return 0;
    
    el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
//    LOG(INFO) << "Hello World!";
    
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
