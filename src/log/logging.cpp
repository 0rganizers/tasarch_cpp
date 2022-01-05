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
#include <fmt/core.h>

namespace tasarch::log {
    static std::vector<spdlog::sink_ptr> sink_list;

    auto setup_logging() -> void
    {
        // TODO: configuration
        // TODO: Add thread name and uptime
#define BASE_PATTERN "{}%Y-%m-%d %H:%M:%S.%e (%3oms) [%-25n] %23s:%-5# %8l|{} %v{}"
        // contains the base pattern, that is (potentially) expanded with color formatters.
        std::string no_color_pattern = fmt::format(BASE_PATTERN, "", "", "");
        std::string color_pattern = fmt::format(BASE_PATTERN, "%1|", "%=1|", "%-1|");
        
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);
        auto formatter = std::make_unique<spdlog::pattern_formatter>();
        formatter->add_flag<color_msg_part>('|');
        formatter->set_pattern(color_pattern);
        console_sink->set_formatter(std::move(formatter));
        add_sink(console_sink);
        
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("tasarch.log", false);
        file_sink->set_pattern(no_color_pattern);
        file_sink->set_level(spdlog::level::trace);
        add_sink(file_sink);
        
        auto syslog_sink = std::make_shared<spdlog::sinks::syslog_sink_mt>("tasarch", 0, LOG_USER, false);
        syslog_sink->set_level(spdlog::level::info);
        add_sink(syslog_sink);
    }

    auto get(std::string name) -> std::shared_ptr<Logger>
    {
        // TODO: configuration here!
        auto log = std::make_shared<Logger>(name, sink_list.begin(), sink_list.end());
        return log;
    }

    auto add_sink(spdlog::sink_ptr sink) -> void
    {
        sink_list.push_back(sink);
    }
} // namespace tasarch::log
