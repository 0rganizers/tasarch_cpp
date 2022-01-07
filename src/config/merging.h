#ifndef __MERGING_H
#define __MERGING_H

#include "common.h"

namespace tasarch::config {
    auto merge_values(toml::value& base, toml::value& addition) -> toml::value;
} // namespace tasarch::config

#endif /* __MERGING_H */
