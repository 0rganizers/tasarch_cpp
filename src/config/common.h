#ifndef __COMMON_H
#define __COMMON_H

#define TOML11_PRESERVE_COMMENTS_BY_DEFAULT 1
#define TOML11_COLORIZE_ERROR_MESSAGE 1
#include <toml.hpp>

namespace tasarch::config {
    static inline auto parse_toml(const std::string& toml) -> toml::value
    {
        std::istringstream inp(toml);
        return toml::parse(inp);
    }
} // namespace tasarch::config

#endif /* __COMMON_H */
