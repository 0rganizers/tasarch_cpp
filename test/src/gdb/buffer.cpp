#include <array>
#include <stdexcept>
#include <string>
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

ut::suite buffer_tests = []{
    using namespace ut;
    using asio::ip::tcp;
    using tasarch::gdb::buffer;

    auto config_val = tasarch::config::parse_toml("logging.test.gdb.level = 'trace'\nlogging.gdb.level = 'trace'");
    tasarch::config::conf()->load_from(config_val);
    auto logger = tasarch::log::get("test.gdb");

    "conversion tests"_test = [&]{
        buffer buf(128);
        logger->info("default sizes: {}, {}", buf.read_size(), buf.write_size());
        std::string initial = "testing";
        buf.append_buf(initial);
        auto all = buf.read_buf<std::string>();
        logger->info("contents: {}", all);
        expect(all == initial);
        auto mutabl = buf.read_buf<asio::mutable_buffer>();
        buf.append_buf(mutabl);
        all = buf.read_buf<std::string>();
        expect(all == initial + initial);
    };
};