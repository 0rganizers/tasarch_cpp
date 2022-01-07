#include <ut/ut.hpp>
#include <utility>
#include "log/logging.h"
#include "config/common.h"
#include "config/merging.h"

namespace ut = boost::ut;

auto toml_throw_error(std::string msg, toml::value& lhs, toml::value& rhs) -> void
{
    throw toml::internal_error(toml::format_error(msg, lhs, "lhs here", rhs, "rhs here"), lhs.location());
}

auto toml_equal(toml::value& lhs, toml::value& rhs) -> void
{
    if(lhs.type()     != rhs.type())
    {
        toml_throw_error("types do not match", lhs, rhs);
    }

    switch(lhs.type())
    {
        case toml::value_t::array    :
        {

        }
        break;
        case toml::value_t::table    :
        {
            for (auto [key, val] : lhs.as_table()) {
                if (!rhs.contains(key))
                {
                    toml_throw_error("rhs does not contain key " + key, val, rhs);
                }
                auto rhs_val = rhs[key];
                toml_equal(val, rhs_val);
            }

            for (auto [key, val] : rhs.as_table()) {
                if (!lhs.contains(key))
                {
                    toml_throw_error("lhs does not contain key " + key, lhs, val);
                }
                auto lhs_val = lhs[key];
                toml_equal(lhs_val, val);
            }
        }
        break;
        case toml::value_t::empty:
        break;
        default:
        {
            if (lhs != rhs)
            {
                toml_throw_error("values do not match", lhs, rhs);
            }
        }
    }
}

ut::suite merging = [] {
    using namespace ut;
    using namespace tasarch::config;
    
    auto logger = tasarch::log::get("test.merging");

    auto do_merge = [](std::string base_toml, std::string addition_toml) {
        auto base = parse_toml(std::move(base_toml));
        auto addition = parse_toml(std::move(addition_toml));
        auto merged = merge_values(base, addition);
        return merged;
    };

    auto expect_match = [](toml::value& lhs, toml::value& rhs) {
        toml_equal(lhs, rhs);
        // expect(nothrow([&lhs, &rhs] {
            
        // })) << "expected lhs and rhs to match!";
    };

    struct merge_test_data {
        std::string name;
        std::string base;
        std::string addition;
        std::string result;
    };

    std::vector<merge_test_data> valid_test_cases = {
        {
            "Simple merge test",
            R"([table]
value = "asdf")",
            R"(table.value = "asdf2")",
            R"(table.value = "asdf2")"
        },
        {
            "Complex Merger",
            R"([table]
value.subvalue = "asdf"
value.subvalue2 = 42

[table2]
value = [1, 2, 3]

[table3]
value = [{ pi = 3.14 }, { e = 2.888 }])",
            R"(table.value = {subvalue = "asdf2"}
table2.value = [4, 5]
table2.other = "asdf"
table3.value = [{ pi = 3.24 }, { notpi = 3.14 }])",
            R"([table]
value.subvalue = "asdf2"
value.subvalue2 = 42

[table2]
value = [1,2,3,4,5]
other = "asdf"

[table3]
value = [{pi = 3.14}, {e = 2.888}, {pi = 3.24}, {notpi = 3.14}])"
        }
    };

    for (auto const tc : valid_test_cases) {
        test("merge test: " + tc.name) = [tc, do_merge, expect_match] {
            auto merged = do_merge(tc.base, tc.addition);
            auto expected = parse_toml(tc.result);
            expect_match(merged, expected);
            // expect(merged == expected) << "result and expected do not match!";
        };
    }
    
    struct merge_error_test {
        std::string name;
        std::string base;
        std::string addition;
    };

    std::vector<merge_error_test> invalid_test_cases = {
        {
            "Invalid nested types",
            R"([table]
value = "asdf")",
            R"(table.value = 42)",
        }
    };


    for (auto const tc : invalid_test_cases) {
        test("invalid merge test: " + tc.name) = [tc, do_merge] {
            expect(throws([tc, do_merge] {
                do_merge(tc.base, tc.addition);
            }));
        };
    }
};
