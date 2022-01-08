#ifndef __MERGING_H
#define __MERGING_H

#include "common.h"

namespace tasarch::config {
    /**
     * @brief Merges config values found in addition "onto" base. This is used to achieve something similar to vscode, where you have a user config and a workspace config.
     * 
     * The example below shows a resulting merge
     * @code {.toml}
     * # base.toml
     * [table]
     * value.subvalue = "asdf"
     * value.subvalue2 = 42
     * [table2]
     * value = [1, 2, 3]
     * 
     * # addition.toml
     * table.value.subvalue = "asdf2"
     * table2.value = [4, 5]
     * table2.newval = 13.3
     *
     * # merged.toml
     * [table]
     * value.subvalue = "asdf2"
     * value.subvalue2 = 42
     * [table2]
     * value = [1, 2, 3, 4, 5]
     * newval = 13.3
     * @endcode
     * 
     * @param base The base config to go from.
     * @param addition The additional config values to merge onto.
     * @return toml::value The merged config
     */
    auto merge_values(toml::value& base, toml::value& addition) -> toml::value;
} // namespace tasarch::config

#endif /* __MERGING_H */
