#ifndef __LOGGING_H
#define __LOGGING_H

#include <spdlog/spdlog.h>
#include "logger.h"

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
