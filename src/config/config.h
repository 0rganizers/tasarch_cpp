#ifndef __CONFIG_H
#define __CONFIG_H

#include "common.h"
#include <spdlog/common.h>
#include <spdlog/details/registry.h>
#include "log/logging.h"

namespace toml {
    /**
     * @brief Needed to allow easy conversion from toml to spdlog::level. Unfortunately, I did not actually get this to work lul.
     * 
     * @tparam  
     */
    template<>
    struct from<spdlog::level::level_enum>
    {
        /**
         * @brief A list of all possible level names, so we can augment the error message to be more helpful :)
         * 
         */
        const static inline std::vector<spdlog::string_view_t> all_names = SPDLOG_LEVEL_NAMES;

        /**
         * @brief Convert toml value to spdlog::level.
         * 
         * @throws toml::type_error or toml::internal_error
         * @param v 
         * @return spdlog::level::level_enum 
         */
        static auto from_toml(const value& v) -> spdlog::level::level_enum
        {
            if (!v.is_string())
            {
                throw toml::type_error(toml::format_error("cannot convert non string to logging level", v, "invalid type here", {}, /*colorize=*/true), v.location());
            }

            std::string lvl_str = v.as_string().str;
            std::transform(lvl_str.begin(), lvl_str.end(), lvl_str.begin(),
    [](unsigned char c){ return std::tolower(c); });
            auto ret = spdlog::level::from_str(lvl_str);
            
            if (ret == spdlog::level::off)
            {
                if (lvl_str != "off")
                {
                    std::string allowed;
                    for (auto name : all_names)
                    {
                        allowed += fmt::to_string(name) + " ";
                    }
                    throw toml::internal_error(toml::format_error("invalid name for logging level, allowed names are: " + allowed, v, "invalid name here", {}, true), v.location());
                }
            }

            return ret;
        }
    };
} // namespace toml

namespace tasarch::config {
    /**
     * @brief This utility struct is used to hold the hierarchy of logging levels defined in a config.
     * 
     * Lets take the following config for example:
     * @code {.toml}
     * [logging]
     * level = "info"
     * parent.level = "trace"
     * parent.child.level = "err"
     * @endcode
     * 
     * The resulting struct could be initialized as follows:
     * @code {.cpp}
     log_levels root = {.level = info, .children = {
         {"parent": {.level = trace, .children = {
             "child": {.level = error, .children = {}}
         }}}
     }};
     * @endcode
     * 
     * 
     */
    struct log_levels {
        log_levels() = default;

        spdlog::level::level_enum level = spdlog::level::info;
        std::unordered_map<std::string, log_levels> children;

        /**
         * @brief Load hierarchy from the given toml value.
         * 
         * @param v 
         */
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

        /**
         * @brief (recursively) scans hierarchy of logging levels, to find best match for name.
         * 
         * For example, we have the following config:
         * @code {.toml}
         * [logging]
         * level = "info"
         * parent.level = "trace"
         * parent.child.level = "err"
         * @endcode
         * 
         * A logger named `parent.child` will have a log level of `err`, one named `parent.child2` will have `trace`.
         * A logger named `parent2` will have a log level of `info` 
         *
         * @param name Hierarchical name of log level to retrieved, where each level in the hierarchy is separated by a `.`.
         * @return spdlog::level::level_enum 
         */
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

    /**
     * @brief Holds all configuration regarding logging.

     * @todo For now this only holds the `log_levels`, update for more configuration in the future such as:
     * - configuring different sinks
     * - ???
     * 
     */
    struct logging {

        log_levels levels;

        /**
         * @brief Load the logging config from the toml value. 
         * @note This also applies the newly loaded levels to all loggers!
         * @todo Better way to refresh config?
         * @param v 
         */
        void load_from(const toml::value& v)
        {
            this->levels.from_toml(v);
            log::apply_all([this](std::shared_ptr<log::logger> logger)
            {
                logger->set_level(this->levels.get_level(logger->name()));
            });
        }
    };

    /**
     * @brief Root configuration object. Currently only holds logging config.
     * @todo add much more config.
     */
    struct config {
        logging logging;

        /**
         * @brief Loads the whole configuration from the given toml value.
         * @note For some configuration, this might trigger a reloading, such as for logging.
         * 
         * @param v 
         */
        void load_from(const toml::value& v)
        {
            this->logging.load_from(toml::find_or(v, "logging", toml::table()));
        }

        /**
         * @brief Reloads the configuration from disk, if it can be parsed.
         * @todo Merge user and workspace config.
         */
        void reload()
        {
            auto logger = log::get("config");
            try {
                logger->info("Reloading config from $PWD/tasarch.toml");
                auto config_val = toml::parse("tasarch.toml");
                this->load_from(config_val);
                logger->info("Loaded config from $PWD/tasarch.toml");
            } catch (std::exception &e) {
                logger->error("Failed to load config from $PWD/tasarch.toml, using default config!:\n{}", e.what());
            }
        }
    };

    /**
     * @brief The globally shared root configuration object.
     * @todo Figure out a better way? especially with reloading? Can we just take references to individual fields?
     * 
     */
    extern config conf;
} // namespace tasarch::config

#endif /* __CONFIG_H */
