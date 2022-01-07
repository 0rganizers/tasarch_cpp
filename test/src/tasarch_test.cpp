#include <ut/ut.hpp>
#include "log/logging.h"

auto main() -> int
{
    tasarch::log::setup_logging();
    auto test_logger = tasarch::log::get("test");
    test_logger->info("Successfully initialized logging!");
    // We need to explicitly run tests here, otherwise, it will be executed during exit, where the loggers are already destructed!
    boost::ut::cfg<>.run();
    return 0;
}
