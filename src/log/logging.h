#ifndef __LOGGING_H
#define __LOGGING_H

#include <spdlog/spdlog.h>
#include "logger.h"
// Include all formatters, so hopefully they are always available :)
#include "asio_formatters.h"

#define TIME_PATTERN "%Y-%m-%d %H:%M:%S.%e"
#define OFFSET_PATTERN "(%3oms)"
#define NAME_PATTERN "[%-25n]"
#define LOC_PATTERN "%23s:%-5#"
#define LVL_PATTERN "%8l"

// TODO: Add thread name and uptime
#define BASE_PREAMBLE_PATTERN TIME_PATTERN " " OFFSET_PATTERN " " NAME_PATTERN " " LOC_PATTERN " " LVL_PATTERN

#define COLOR_PATTERN "%1|" BASE_PREAMBLE_PATTERN "|%=1| %v%-1|"
#define NO_COLOR_PATTERN BASE_PREAMBLE_PATTERN "| %v"

namespace tasarch::log {
    /// Setup logging by creating some default sinks and other important things.
    void setup_logging();
    
    /// Adds the given sink to the list of sinks to be used for any logger.
    /// @note This will not be applied retroactively, so make sure to add all sinks before you start using loggers!
    void add_sink(spdlog::sink_ptr sink);

    /// Gets a named logger. Use this wherever you can, since it allows for hierarchical setting of level!
    /// @param name The name. A hierarchy is created with "." syntax.
    std::shared_ptr<Logger> get(std::string name);

    void apply_all(const std::function<void(const std::shared_ptr<Logger>)> &fun);

    class WithLogger {
    public:
        WithLogger(std::string name) : logger(get(name)) {}
    protected:
        std::shared_ptr<Logger> logger;
    };
} // namespace tasarch::log



#endif /* __LOGGING_H */
