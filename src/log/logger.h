//
//  logger.h
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

/**
    @file logger.h
    @brief Contains a custom subclass of spdlog:logger and necessary other things to support source_location from C++20.
 
    Most of the good stuff here is thanks to: https://github.com/gabime/spdlog/issues/1959
 */

namespace tasarch::log {
    /**
        Not sure why this is actually needed?
        @todo investigate necessity of this?
     */
    [[nodiscard]] constexpr auto get_log_source_location(
        const source_location &location) {
      return spdlog::source_loc{location.file_name(),
                                static_cast<std::int32_t>(location.line()),
                                location.function_name()};
    }

    /**
        Needed for source_location as a default value in variadic functions.
        Dont ask me how exactly this works.
     */
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


    /**
        @brief Custom spdlog::logger subclass, so we can use C++20's source_location.
        We also add a bunch of additional convenience functions.
     */
    class logger : public spdlog::logger {
    public:
        
        /**
            @name Custom Members
            We add some custom members for convencience.
         */
        ///@{
#pragma mark Custom
        std::shared_ptr<logger> child(std::string name);
        ///@}

        
        
        /**
            @name Constructors
            We need to implement all the constructors again, but we can just directly pass the given parameters!
         */
        ///@{
#pragma mark Constructors
        /// Empty logger
        explicit logger(std::string name) : spdlog::logger(name)
        {}

        /// Logger with range on sinks
        template<typename It>
        logger(std::string name, It begin, It end) : spdlog::logger(name, begin, end)
        {}

        /// Logger with single sink
        logger(std::string name, spdlog::sink_ptr single_sink)
            : spdlog::logger(name, single_sink)
        {}

        /// Logger with sinks init list
        logger(std::string name, spdlog::sinks_init_list sinks)
            : spdlog::logger(name, sinks)
        {}
        
        ///@}
        
        
        /**
            @name Variadic Log Functions
            To be able to use source_location, we need to shadow the parent's variadic and non variadic log functions.
         */
        ///@{
#pragma mark Variadic Log Functions
        
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
        
        ///@}
        
        /**
            @name Single Log Message
         */
        ///@{
#pragma mark Single Message
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
        
        ///@}
    };
} // namespace tasarch::log

#endif /* logger_hpp */
