#ifndef __LOGGING_H
#define __LOGGING_H

#include <spdlog/spdlog.h>
#include "logger.h"

#pragma mark Logging Macros
/**
    Since (for now, C++20 when?) we can only get filename and source line information via macros, we need to do all logging via macros.
    spdlog already provides macros for this, but they are long and can be cumbersome to use.
    We define our own here, use these at all times, instead of the loggers directly!
 */

/**
    Log to the given logger with the given level.
    We have more convenience functions for all levels, so don't worry about this too much.
    @note level must be a value of `trace`, `debug`, `info`, `warn`, `err`, `critical`
 */
#define LOG(logger, lvl, ...) SPDLOG_LOGGER_CALL(logger, spdlog::level::lvl, __VA_ARGS__)

/**
    See #LOG(logger, level, ...) for more information.
 */
#define TRACE(logger, ...) LOG(logger, trace, __VA_ARGS__)
#define DEBUG(logger, ...) LOG(logger, debug, __VA_ARGS__)
#define INFO(logger, ...) LOG(logger, info, __VA_ARGS__)
#define WARN(logger, ...) LOG(logger, warn, __VA_ARGS__)
#define ERROR(logger, ...) LOG(logger, err, __VA_ARGS__)
#define CRITICAL(logger, ...) LOG(logger, critical, __VA_ARGS__)

/**
    Convenience function to get a logger prefixed with `tasarch.`.
    For now, always use this!
 */
#define GETLOG(name) tasarch::log::get("tasarch." #name)

/**
 * @brief Class Logger. Use this inside classes to log with less typing :)
 * @note This of course requires you to define a class property named `logger`. Always inherit from WithLogger to ensure you have that :)
 */
#define CLOG(level, ...) LOG(this->logger, level, __VA_ARGS__)

#define CTRACE(...) CLOG(trace, __VA_ARGS__)
#define CDEBUG(...) CLOG(debug, __VA_ARGS__)
#define CINFO(...) CLOG(info, __VA_ARGS__)
#define CWARN(...) CLOG(warn, __VA_ARGS__)
#define CERROR(...) CLOG(err, __VA_ARGS__)
#define CCRITICAL(...) CLOG(critical, __VA_ARGS__)

#pragma mark Logging Functions

namespace tasarch::log {
    /// Setup logging by creating some default sinks and other important things.
    void setup_logging();
    
    /// Adds the given sink to the list of sinks to be used for any logger.
    /// @note This will not be applied retroactively, so make sure to add all sinks before you start using loggers!
    void add_sink(spdlog::sink_ptr sink);

    /// Gets a named logger. Use this wherever you can, since it allows for hierarchical setting of level!
    /// @param name The name. A hierarchy is created with "." syntax.
    std::shared_ptr<logger> get(std::string name);

    class WithLogger {
    public:
        WithLogger(std::string name) : logger(get(name)) {}
    protected:
        std::shared_ptr<logger> logger;
    };
} // namespace tasarch::log



#endif /* __LOGGING_H */
