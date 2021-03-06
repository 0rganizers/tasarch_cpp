#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#include "gdb/bg_executor.h"
#include "gui/mainwindow.h"
#include <QSurfaceFormat>
#include <thread>

#include "log/logging.h"
#include "log/formatters.h"
#include "gui/LogWindow.h"
#include <spdlog/sinks/qt_sinks.h>
#include <toml/parser.hpp>
#include "config/config.h"
#include "gdb/server.h"

auto main(int argc, char* argv[]) -> int
{
    tasarch::log::setup_logging();
    auto root = tasarch::log::get("");
    tasarch::config::conf()->reload();
    root->trace("Config reloaded!");
    root->debug("Debug message!");
    root->info("Info message!");
    root->warn("Warn message!");
    root->error("Error message!");
    root->critical("Critical message!");

    tasarch::gdb::bg_executor::instance().start();

    auto server = std::make_shared<tasarch::gdb::server>(nullptr);
    server->start();
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    server->stop();

    tasarch::gdb::bg_executor::instance().stop();
    return 0;
    
    auto logger = tasarch::log::get("tasarch");
    logger->info("Initializing QT application...");
    
    // needs to be here, before we actually create the GUI!
    QSurfaceFormat form;
    form.setMajorVersion(4);
    form.setMinorVersion(1);
    form.setRenderableType(QSurfaceFormat::OpenGL);
    form.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(form);
    
    
    QApplication app(argc, argv);
    
    // Needs to be created as early as possible, so we can log as early as possible!
    auto logw = new tasarch::gui::LogWindow();
    
    auto logw_sink = std::make_shared<spdlog::sinks::qt_sink_mt>(logw, "append");
    // TODO: configuration / maybe an option on the window itself? but then we would have to log everything and hide them?
    // TODO: more in depth log viewer using a table view with filtering options?
    logw_sink->set_level(spdlog::level::debug);
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<tasarch::log::qt_html_color_msg_part>('|');
    formatter->set_pattern(COLOR_PATTERN);
    logw_sink->set_formatter(std::move(formatter));
    tasarch::log::add_sink(logw_sink);
    
    auto window = new tasarch::gui::MainWindow();
    
    window->show();
    logw->show();
    
    window->add_window(logw);
 
    logw->testAppend();
    logger->info("Info message!");
    logger->warn("Warn message!");
    logger->error("Error message!");
    logger->critical("Critical message!");
    
    return app.exec();
}
