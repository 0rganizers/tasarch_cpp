//
//  formatters.h
//  tasarch
//
//  Created by Leonardo Galli on 03.01.22.
//

#ifndef formatters_h
#define formatters_h

#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <fmt/color.h>

namespace tasarch::log {
    /// Automatically adds colors to a message. By abusing the padding info, this allows for coloring in two parts.
    /// Anything Info or below, has first part dimmed, second part normal.
    /// Anything above Info, has all the same color.
    class color_msg_part : public spdlog::custom_flag_formatter
    {
        inline const static std::shared_ptr<spdlog::sinks::ansicolor_stderr_sink_st> ansi_sink = std::make_shared<spdlog::sinks::ansicolor_stderr_sink_st>();
        
        auto format(const spdlog::details::log_msg &msg, const std::tm &, spdlog::memory_buf_t &dest) -> void override
        {
            // TODO: Maybe we want trace and debug second part to also change color?
            spdlog::string_view_t res;
            spdlog::string_view_t level_color;
            
            switch (msg.level) {
                case spdlog::level::trace:
                    level_color = ansi_sink->white;
                    break;
                case spdlog::level::debug:
                    level_color = ansi_sink->white;
                    break;
                case spdlog::level::info:
                    level_color = ansi_sink->white;
                    break;
                case spdlog::level::warn:
                    level_color = ansi_sink->yellow_bold;
                    break;
                case spdlog::level::err:
                    level_color = ansi_sink->red_bold;
                    break;
                case spdlog::level::critical:
                    level_color = ansi_sink->bold_on_red;
                    break;
            }
            
            switch (this->padinfo_.side_) {
                case spdlog::details::padding_info::pad_side::left:
                {
                    if (msg.level <= spdlog::level::info) {
                        res = ansi_sink->dark;
                    } else {
                        res = level_color;
                    }
                }
                    break;
                    
                case spdlog::details::padding_info::pad_side::center:
                {
                    if (msg.level <= spdlog::level::info) {
                        res = ansi_sink->reset;
                    }
                    // if we are not info, we do not do anything here!
                }
                    break;
                    
                case spdlog::details::padding_info::pad_side::right:
                    res = ansi_sink->reset;
                    break;
            }
            dest.append(res.begin(), res.end());
        }
        
        auto clone() const -> std::unique_ptr<spdlog::custom_flag_formatter> override
        {
            return spdlog::details::make_unique<color_msg_part>();
        }
    };
} // namespace tasarch::log

#endif /* formatters_h */
