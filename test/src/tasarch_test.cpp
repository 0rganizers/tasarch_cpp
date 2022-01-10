#include <iomanip>
#include <iostream>
#include <ostream>
#include <ut/ut.hpp>
#include "log/logging.h"
#include "gdb/bg_executor.h"

namespace ut = boost::ut;

namespace cfg {
    class printer : public ut::printer {
    public:
        template <class T>
        auto& operator<<(const T& t) {
            std::cout << ut::detail::get(t) << std::flush;
            out_ << ut::detail::get(t);
            return *this;
        }
    };

    template <class TPrinter = printer>
    class reporter : public ut::reporter<TPrinter> {
    public:
        auto on(ut::events::test_begin test_begin) -> void {
            this->printer_ << std::setw(80) << std::left << std::setfill('*') << "" << "\n";
            this->printer_ << "*" << "Running \"" << std::string(test_begin.name) + "\"..." << "\n";
            this->printer_ << std::setw(80) << std::left << std::setfill('*') << "" << "\n";
            this->fails_ = this->asserts_.fail;
        }

        auto on(ut::events::summary sum) -> void {
            ut::reporter<TPrinter>::on(sum);
        }
    };
} // namespace cfg
/*
template <>
auto ut::cfg<ut::override> = ut::runner<ut::reporter<cfg::printer>>{};*/

auto main() -> int
{
    tasarch::log::setup_logging();
    auto test_logger = tasarch::log::get("test");
    test_logger->info("Successfully initialized logging!");
    // we need to start our bg executor!
    tasarch::gdb::bg_executor::instance().start();
    // We need to explicitly run tests here, otherwise, it will be executed during exit, where the loggers are already destructed!
    boost::ut::cfg<>.run();
    // ut::cfg<>.run();
    // Now stop!
    tasarch::gdb::bg_executor::instance().stop();
    return 0;
}
