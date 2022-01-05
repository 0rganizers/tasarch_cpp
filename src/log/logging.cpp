//
//  logging.cpp
//  tasarch
//
//  Created by Leonardo Galli on 03.01.22.
//

#include "logging.h"
#include "formatters.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/syslog_sink.h>
#include <spdlog/sinks/dist_sink.h>
#include <fmt/core.h>

namespace tasarch::log {
    // TODO: Do we really still need this?
    static std::vector<spdlog::sink_ptr> sink_list;
    static std::shared_ptr<spdlog::sinks::dist_sink_mt> dist_sink = nullptr;

    auto setup_logging() -> void
    {
        // TODO: configuration
        
        
        // to be able to dynamically add sinks later in live and also potentially update them, we use only a dist_sink for every logger
        // we can then add new sinks to the dist_sink and magically new log messages will be sent to it
        
        dist_sink = std::make_shared<spdlog::sinks::dist_sink_mt>();
        
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::debug);
        auto formatter = std::make_unique<spdlog::pattern_formatter>();
        formatter->add_flag<ansi_color_msg_part>('|');
        formatter->set_pattern(COLOR_PATTERN);
        console_sink->set_formatter(std::move(formatter));
        add_sink(console_sink);
        
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("tasarch.log", false);
        file_sink->set_pattern(NO_COLOR_PATTERN);
        file_sink->set_level(spdlog::level::trace);
        add_sink(file_sink);
        
        auto syslog_sink = std::make_shared<spdlog::sinks::syslog_sink_mt>("tasarch", 0, LOG_USER, false);
        syslog_sink->set_level(spdlog::level::info);
        add_sink(syslog_sink);
    }

    auto get(std::string name) -> std::shared_ptr<logger>
    {
        // TODO: configuration here!
        auto log = std::make_shared<logger>(name, dist_sink);
        // sinks should be deciding what to log!
        log->set_level(spdlog::level::trace);
        return log;
    }

    auto add_sink(spdlog::sink_ptr sink) -> void
    {
        sink_list.push_back(sink);
        dist_sink->add_sink(sink);
    }
} // namespace tasarch::log
