#ifndef __CONFIG_H
#define __CONFIG_H

#include <spdlog/common.h>
#include <spdlog/details/registry.h>
#include <toml/from.hpp>
#include <toml/get.hpp>
#include "common.h"
#include "log/logging.h"

namespace toml {
    template<>
    struct from<spdlog::level::level_enum>
    {
        static auto from_toml(const value& v) -> spdlog::level::level_enum
        {
            if (!v.is_string())
            {
                throw toml::type_error(toml::format_error("cannot convert non string to logging level", v, "invalid type here"), v.location());
            }

            std::string lvl_str = v.as_string().str;
            std::transform(lvl_str.begin(), lvl_str.end(), lvl_str.begin(),
    [](unsigned char c){ return std::tolower(c); });
            auto ret = spdlog::level::from_str(lvl_str);
            
            if (ret == spdlog::level::off)
            {
                if (lvl_str != "off")
                {
                    throw toml::internal_error(toml::format_error("invalid name for logging level", v, "invalid name here"), v.location());
                }
            }

            return ret;
        }
    };
} // namespace toml

namespace tasarch::config {
    struct log_levels {
        log_levels() = default;

        spdlog::level::level_enum level = spdlog::level::info;
        std::unordered_map<std::string, log_levels> children;

        void from_toml(const toml::value& v)
        {
            // parse this level.
            if (v.contains("level"))
            {
                auto level_val = v.at("level");
                this->level = toml::from<spdlog::level::level_enum>::from_toml(level_val);
            } else {
                this->level = spdlog::level::info;
            }
            
            // remove any existing children.
            children.clear();
            for (auto [child, val] : v.as_table()) {
                if (child == "level") continue;
                children[child].from_toml(val);
            }
        }

        auto get_level(const std::string& name) -> spdlog::level::level_enum
        {
            if (name.empty()) return this->level;

            size_t pos = name.find('.');
            // If not found, we split at the very end :)
            if (pos == std::string::npos) pos = name.length();
            
            std::string parent = name.substr(0, pos);
            std::string child;
            if (pos < name.length()) { child = name.substr(pos+1); }

            if (this->children.contains(parent))
            {
                return this->children[parent].get_level(child);
            }

            return this->level;
        }
    };

    struct logging {

        log_levels levels;

        void load_from(const toml::value& v)
        {
            this->levels.from_toml(v);
            log::apply_all([this](std::shared_ptr<log::logger> logger)
            {
                logger->set_level(this->levels.get_level(logger->name()));
            });
        }
    };

    struct config {
        logging logging;

        void load_from(const toml::value& v)
        {
            this->logging.load_from(toml::find_or(v, "logging", toml::table()));
        }
    };

    extern config conf;
} // namespace tasarch::config

#endif /* __CONFIG_H */
