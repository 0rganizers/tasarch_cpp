//
//  logger.cpp
//  tasarch
//
//  Created by Leonardo Galli on 04.01.22.
//

#include "logger.h"
#include "logging.h"

namespace tasarch::log {
    std::shared_ptr<logger> logger::child(std::string name)
    {
        std::string full_name = this->name() + "." + name;
        return get(full_name);
    }
} // namespace tasarch::log
