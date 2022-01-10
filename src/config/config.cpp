#include <memory>
#include "config.h"

namespace tasarch::config {
    auto config::instance() -> conf_ptr
    {
        return conf();
    }

    auto conf() -> conf_ptr
    {
        static conf_ptr conf(new config);
        return conf;
    }
} // namespace tasarch::config