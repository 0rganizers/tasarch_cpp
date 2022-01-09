#include <ut/ut.hpp>
#include "log/logging.h"
#include "gdb/bg_executor.h"

auto main() -> int
{
    tasarch::log::setup_logging();
    auto test_logger = tasarch::log::get("test");
    test_logger->info("Successfully initialized logging!");
    // we need to start our bg executor!
    tasarch::gdb::bg_executor::instance().start();
    // We need to explicitly run tests here, otherwise, it will be executed during exit, where the loggers are already destructed!
    boost::ut::cfg<>.run();
    // Now stop!
    tasarch::gdb::bg_executor::instance().stop();
    return 0;
}
