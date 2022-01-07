#include <iterator>
#include "merging.h"
#include <toml/exception.hpp>

namespace tasarch::config {
    auto merge_values(toml::value& base, toml::value& addition) -> toml::value
    {
        if (base.type() != addition.type())
        {
            throw toml::type_error(toml::format_error("cannot merge values of different types", base, "value from base config", addition, "value from additional config"), base.location());
        }

        switch (base.type()) {
        /**
         * @todo Figure out how we could make this work better. Is this really how we want the merging to be?
         */
        case toml::value_t::array:
        {
            toml::value ret(base);
            ret.as_array().insert(std::end(ret.as_array()), std::begin(addition.as_array()), std::end(addition.as_array()));
            return ret;
        }
        break;

        case toml::value_t::table:
        {
            toml::value ret(base);
            for (auto& [key, val]: addition.as_table()) {
                if (base.contains(key)) {
                    ret[key] = merge_values(base[key], val);
                } else {
                    ret[key] = val;
                }
            }
            return ret;
        }
        break;

        // by default we can just return addition!
        default:
        return addition;
        break;
        }

        return base;
    }
} // namespace tasarch::config
