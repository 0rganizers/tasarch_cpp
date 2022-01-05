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
    public:
        auto format(const spdlog::details::log_msg &msg, const std::tm &, spdlog::memory_buf_t &dest) -> void override
        {
            // TODO: Maybe we want trace and debug second part to also change color?
            spdlog::string_view_t res;
            spdlog::string_view_t level_color = this->level_color(msg.level);
            
            switch (this->padinfo_.side_) {
                case spdlog::details::padding_info::pad_side::left:
                {
                    if (msg.level <= spdlog::level::info) {
                        res = this->get_msg_part(msg_part::preamble_start);
                    } else {
                        res = level_color;
                    }
                }
                    break;
                    
                case spdlog::details::padding_info::pad_side::center:
                {
                    if (msg.level <= spdlog::level::info) {
                        res = this->get_msg_part(msg_part::preamble_end);
                    }
                    // if we are not info, we do not do anything here!
                }
                    break;
                    
                case spdlog::details::padding_info::pad_side::right:
                    res = this->get_msg_part(msg_part::msg_end);
                    break;
            }
            dest.append(res.begin(), res.end());
        }
        
        auto clone() const -> std::unique_ptr<spdlog::custom_flag_formatter> override
        {
            return spdlog::details::make_unique<color_msg_part>();
        }
        
    protected:
        enum msg_part
        {
            preamble_start,
            preamble_end,
            msg_end
        };
        
        virtual auto level_color(spdlog::level::level_enum level) -> spdlog::string_view_t
        {
            return "";
        }
        
        virtual auto get_msg_part(msg_part part) -> spdlog::string_view_t
        {
            return "";
        }
    };

    class ansi_color_msg_part : public color_msg_part
    {
        inline const static std::shared_ptr<spdlog::sinks::ansicolor_stderr_sink_st> ansi_sink = std::make_shared<spdlog::sinks::ansicolor_stderr_sink_st>();

        auto clone() const -> std::unique_ptr<spdlog::custom_flag_formatter> override
        {
            return spdlog::details::make_unique<ansi_color_msg_part>();
        }
        
        auto level_color(spdlog::level::level_enum level) -> spdlog::string_view_t override
        {
            switch (level) {
                case spdlog::level::trace:
                    return ansi_sink->white;
                case spdlog::level::debug:
                    return ansi_sink->white;
                case spdlog::level::info:
                    return ansi_sink->white;
                case spdlog::level::warn:
                    return ansi_sink->yellow_bold;
                case spdlog::level::err:
                    return ansi_sink->red_bold;
                case spdlog::level::critical:
                    return ansi_sink->bold_on_red;
            }
            
            return "";
        }
        
        auto get_msg_part(msg_part part) -> spdlog::string_view_t override
        {
            switch (part) {
                case msg_part::preamble_start:
                    return ansi_sink->dark;
                default:
                    return ansi_sink->reset;
            }
        }
    };

    class qt_html_color_msg_part : public color_msg_part
    {
        auto clone() const -> std::unique_ptr<spdlog::custom_flag_formatter> override
        {
            return spdlog::details::make_unique<qt_html_color_msg_part>();
        }
        
        auto level_color(spdlog::level::level_enum level) -> spdlog::string_view_t override
        {
            spdlog::string_view_t color = "white";
            spdlog::string_view_t face = "normal";
            spdlog::string_view_t attr = "";
            switch (level) {
                case spdlog::level::trace:
                case spdlog::level::debug:
                case spdlog::level::info:
                    attr = "color: white;";
                    break;
                case spdlog::level::warn:
                    color = "yellow";
                    attr = "color: yellow;";
                    break;
                case spdlog::level::err:
                    color = "red";
                    face = "bold";
                    attr = "color: red; font-weight: bold";
                    break;
                case spdlog::level::critical:
                    color = "red";
                    face = "bold";
                    attr = "color: white; background-color: red; font-weight: bold";
                    break;
            }
            
//            return fmt::format(R"(<pre><font color="{}" face="{}">)", color, face);
            return fmt::format(R"(<pre><span style="{}">)", attr);
        }
        
        auto get_msg_part(msg_part part) -> spdlog::string_view_t override
        {
            switch (part) {
                case msg_part::preamble_start:
                    return R"(<pre><span style="color: #bbbbbb">)";
                case msg_part::preamble_end:
                    return R"(</span><span style="color: white">)";
                case msg_part::msg_end:
                    return "</span></pre>";
            }
        }
    };
} // namespace tasarch::log

#endif /* formatters_h */
