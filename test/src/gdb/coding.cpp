#include "gdb/coding.h"
#include <array>
#include <stdexcept>
#include <string>
#include <type_traits>
#include "gdb/common.h"
#include "gdb/packet_io.h"
#include "log/logging.h"
#include "tcp_server_client_test.h"
#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>
#include <ut/ut.hpp>
#include "config/config.h"
#include "async_test.h"
#include <asio/basic_waitable_timer.hpp>
#include "gdb/buffer.h"

namespace ut = boost::ut;
namespace gdb = tasarch::test::gdb;

ut::suite coding_tests = []{
    using namespace ut;
    using asio::ip::tcp;
    using namespace tasarch::gdb;

    auto config_val = tasarch::config::parse_toml("logging.test.gdb.level = 'trace'\nlogging.gdb.level = 'trace'");
    tasarch::config::conf()->load_from(config_val);
    auto logger = tasarch::log::get("test.gdb");

    "Simple StrCoder test"_test = []{
        
    };
};