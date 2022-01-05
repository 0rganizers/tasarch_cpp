//
//  logger.hpp
//  tasarch
//
//  Created by Leonardo Galli on 04.01.22.
//

#ifndef logger_hpp
#define logger_hpp

#include <spdlog/spdlog.h>
#include <iostream>
#include <string_view>
#include "source_location.h"

namespace tasarch::log {
    // thanks to https://github.com/gabime/spdlog/issues/1959 this should actually work?
    // does some stuff lul, idk exactly why or how this works.
    [[nodiscard]] constexpr auto get_log_source_location(
        const source_location &location) {
      return spdlog::source_loc{location.file_name(),
                                static_cast<std::int32_t>(location.line()),
                                location.function_name()};
    }

    struct format_with_location {
    public:
      std::string_view value;
      spdlog::source_loc loc;

      template <typename String>
      constexpr format_with_location(
          const String &s,
          const source_location &location = source_location::current())
          : value{s}, loc{get_log_source_location(location)} {}
    };

    class Logger : public spdlog::logger {
    public:
        // Empty logger
        explicit Logger(std::string name) : spdlog::logger(name)
        {}

        // Logger with range on sinks
        template<typename It>
        Logger(std::string name, It begin, It end) : spdlog::logger(name, begin, end)
        {}

        // Logger with single sink
        Logger(std::string name, spdlog::sink_ptr single_sink)
            : spdlog::logger(name, single_sink)
        {}

        // Logger with sinks init list
        Logger(std::string name, spdlog::sinks_init_list sinks)
            : spdlog::logger(name, sinks)
        {}
        
        template<typename... Args>
        void trace(format_with_location fmt, Args &&...args)
        {
            this->log_(fmt.loc, spdlog::level::trace, fmt.value, std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        void debug(format_with_location fmt, Args &&...args)
        {
            this->log_(fmt.loc, spdlog::level::debug, fmt.value, std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        void info(format_with_location fmt, Args &&...args)
        {
            this->log_(fmt.loc, spdlog::level::info, fmt.value, std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        void warn(format_with_location fmt, Args &&...args)
        {
            this->log_(fmt.loc, spdlog::level::warn, fmt.value, std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        void error(format_with_location fmt, Args &&...args)
        {
            this->log_(fmt.loc, spdlog::level::err, fmt.value, std::forward<Args>(args)...);
        }
        
        template<typename... Args>
        void critical(format_with_location fmt, Args &&...args)
        {
            this->log_(fmt.loc, spdlog::level::critical, fmt.value, std::forward<Args>(args)...);
        }
        
        // single message
        // TODO: this one
//        void log(spdlog::level::level_enum lvl, spdlog::string_view_t msg, const source_location &location = source_location::current())
//        {
//            this->log(get_log_source_location(location), lvl, msg);
//        }
        
        template<typename T>
        void trace(const T &msg, const source_location &location = source_location::current())
        {
            this->log(get_log_source_location(location), spdlog::level::trace, msg);
        }

        template<typename T>
        void debug(const T &msg, const source_location &location = source_location::current())
        {
            this->log(get_log_source_location(location), spdlog::level::debug, msg);
        }

        template<typename T>
        void info(const T &msg, const source_location &location = source_location::current())
        {
            this->log(get_log_source_location(location), spdlog::level::info, msg);
        }

        template<typename T>
        void warn(const T &msg, const source_location &location = source_location::current())
        {
            this->log(get_log_source_location(location), spdlog::level::warn, msg);
        }

        template<typename T>
        void error(const T &msg, const source_location &location = source_location::current())
        {
            this->log(get_log_source_location(location), spdlog::level::err, msg);
        }

        template<typename T>
        void critical(const T &msg, const source_location &location = source_location::current())
        {
            this->log(get_log_source_location(location), spdlog::level::critical, msg);
        }
    };
} // namespace tasarch::log

#endif /* logger_hpp */
