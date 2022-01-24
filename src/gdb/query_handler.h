#ifndef __QUERY_HANDLER_H
#define __QUERY_HANDLER_H

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <fmt/core.h>
#include "protocol.h"
#include "coding.h"

namespace tasarch::gdb {
    struct query_handler
    {
        std::string name;
        char separator = ':';
        std::optional<std::function<void()>> get_handler = std::nullopt;
        std::optional<std::function<void()>> set_handler = std::nullopt;
        bool advertise = true;

        auto type() -> query_type
        {
            query_type typ = none;
            if (get_handler.has_value()) {
                typ = typ | get_val;
            }
            if (set_handler.has_value()) {
                typ = typ | set_val;
            }
            return typ;
        }
    };

    struct feature
    {
        feature(std::string name, bool supported) : name(std::move(name)), supported(supported) {}
        feature(std::string name, std::string value) : name(std::move(name)), supported(true), value(value) {}
        std::string name;
        std::optional<std::string> value = std::nullopt;
        bool supported = false;
    };

    struct FeatureCoder
    {
        using value_type = feature;
        using loc_type = std::string;

        static auto decode_from(loc_type& loc) -> value_type
        {
            if(loc.back() == '+' || loc.back() == '-') {
                bool supported = loc.back() == '+';
                return feature(loc.substr(0, loc.size()-1), supported);
            }

            auto eq_pos = loc.find('=');
            if (eq_pos == std::string::npos) {
                throw std::runtime_error(fmt::format("Unable to parse feature {}", loc));
            }
            std::string name = loc.substr(0, eq_pos);
            std::string value = loc.substr(eq_pos + 1);
            return feature(name, value);
        }

        static void encode_to(value_type& val, loc_type& loc)
        {
            loc.append(val.name);
            if (val.value.has_value()) {
                loc.append("=");
                loc.append(val.value.value());
            } else {
                loc.append(val.supported ? "+" : "-");
            }
        }
    };

    static_assert(Coding<FeatureCoder>, "Expected FeatureCoder to conform to coding!");
} // namespace tasarch::gdb

#endif /* __QUERY_HANDLER_H */