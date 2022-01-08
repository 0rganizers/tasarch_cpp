#include <exception>
#include <memory>
#include <stdexcept>
#include <spdlog/common.h>
#include <ut/ut.hpp>
#include "config/common.h"
#include "config/config.h"
#include "log/logging.h"
#include <spdlog/sinks/base_sink.h>
#include <set>

template<typename Mutex>
class test_sink : public spdlog::sinks::base_sink <Mutex>
{
public:
    // explicit test_sink() = default;

    auto assert_message(std::string message, const boost::ut::reflection::source_location& loc = boost::ut::reflection::source_location::current())
    {
        auto it = this->messages.find(message);
        return boost::ut::expect(it != this->messages.end(), loc) << "Could not find message" << message;
    }

    auto assert_no_message(std::string message, const boost::ut::reflection::source_location& loc = boost::ut::reflection::source_location::current())
    {
        auto it = this->messages.find(message);
        return boost::ut::expect(it == this->messages.end(), loc) << "Found message" << message;
    }

    void assert_formatted_message(std::string message)
    {
        auto it = this->formatted_messages.find(message);
        if (it == this->formatted_messages.end())
        {
            throw std::runtime_error("Could not find formatted message " + message);
        }
    }

    void clear()
    {
        this->messages.clear();
        this->formatted_messages.clear();
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        this->messages.insert(fmt::to_string(msg.payload));
        // If needed (very likely but not mandatory), the sink formats the message before sending it to its final destination:
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        this->formatted_messages.insert(fmt::to_string(formatted));
    }

    void flush_() override
    {

    }

private:
    std::set<std::string> messages;
    std::set<std::string> formatted_messages;
};

namespace ut = boost::ut;

ut::suite logging = []{
    using namespace ut;

    const std::string info_msg = "INFO MESSAGE";
    const std::string trace_msg = "TRACE MESSAGE";

    "complex config test"_test = [&]{
        auto ts = std::make_shared<test_sink<std::mutex>>();
        ts->set_level(spdlog::level::trace);
        tasarch::log::add_sink(ts);
        auto root = tasarch::log::get("");
        auto parent = tasarch::log::get("parent");
        auto child = tasarch::log::get("parent.child");

        auto test_logger = [ts, info_msg, trace_msg](std::shared_ptr<tasarch::log::logger> logger, spdlog::level::level_enum lvl, const std::string msg = "", const boost::ut::reflection::source_location& loc = boost::ut::reflection::source_location::current()) {
            ts->clear();
            ts->assert_no_message(info_msg, loc) << "even though we just cleared, in case" << msg;

            logger->info(info_msg);
            logger->trace(trace_msg);

            if (lvl <= spdlog::level::trace) {
                ts->assert_message(trace_msg) << msg;
            } else {
                ts->assert_no_message(trace_msg, loc) << msg;
            }

            if (lvl <= spdlog::level::info) {
                ts->assert_message(info_msg) << msg;
            } else {
                ts->assert_no_message(info_msg, loc) << msg;
            }
        };
        
        using spdlog::level::level_enum;

        test_logger(root, level_enum::info);
        test_logger(parent, level_enum::info);
        test_logger(child, level_enum::info);

        // Now we load a config!
        /**
         * @todo Actually use loading functions when they exist.
         */
        auto config_val = tasarch::config::parse_toml("logging.parent.level = 'trace'");
        tasarch::config::conf.load_from(config_val);

        test_logger(root, level_enum::info);
        test_logger(parent, level_enum::trace);
        test_logger(child, level_enum::trace);

        // ensure default is set back!
        config_val = tasarch::config::parse_toml("");
        tasarch::config::conf.load_from(config_val);

        test_logger(root, level_enum::info);
        test_logger(parent, level_enum::info);
        test_logger(child, level_enum::info);

        // now child different from parent!
        config_val = tasarch::config::parse_toml(R"(logging.parent.level = 'trace'
[logging.parent]
child.level = "info")");
        tasarch::config::conf.load_from(config_val);

        test_logger(root, level_enum::info, "root logger for child != parent");
        test_logger(parent, level_enum::trace);
        test_logger(child, level_enum::info);

        // now child different from parent!
        config_val = tasarch::config::parse_toml(R"([logging.parent]
child.level = "error")");
        tasarch::config::conf.load_from(config_val);

        test_logger(root, level_enum::info);
        test_logger(parent, level_enum::info);
        test_logger(child, level_enum::err);
    };
};