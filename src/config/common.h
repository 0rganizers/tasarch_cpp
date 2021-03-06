#ifndef __CONFIG_COMMON_H
#define __CONFIG_COMMON_H

#define TOML11_PRESERVE_COMMENTS_BY_DEFAULT true
#define TOML11_COLORIZE_ERROR_MESSAGE true
#include <toml.hpp>
#include <toml/from.hpp>
#include <toml/get.hpp>

namespace tasarch::config {
    /**
     * @brief Parse toml from a string. Convenience method really.
     * 
     * @param toml 
     * @return toml::value 
     */
    static inline auto parse_toml(const std::string& toml) -> toml::value
    {
        std::istringstream inp(toml);
        return toml::parse(inp);
    }
} // namespace tasarch::config

#endif /* __CONFIG_COMMON_H */
