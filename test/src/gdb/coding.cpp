#include "gdb/coding.h"
#include <array>
#include <functional>
#include <optional>
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

    auto create_buf = [](std::string s){
        buffer buf(0x1000);
        buf.append_buf(s);
        return buf;
    };

    "Simple StrCoder test"_test = [&]{
        auto callback = [](std::string a){
            expect(a == "42");
        };

        std::string s = "42";
        buffer buf(0x1000);
        buf.append_buf(s);
        std::string ret = StrCoder<>::decode_from(buf);
        // std::string error_msg = fmt::format("Unexpected end of string, expected to find {:c} first!", 'c');
        // decode_buffer<StrCoder<>>(buf, callback);
    };

    "optional callback args test"_test = [&]{
        int curr_a = 0x42;
        std::optional<int> curr_b = std::nullopt;
        auto callback = [&](int a, std::optional<int> b){
            logger->info("Callback called with 0x{:x}", a);
            expect(a == curr_a);
            expect(b == curr_b);
        };

        // // verify that normal works
        auto buf = create_buf("42,64");
        curr_b = 0x64;
        decode_buffer<StrCoder<HexNumCoder<int>, ','>, OptCoder<StrCoder<HexNumCoder<int>>>>(buf, callback);
        
        // verify that optional works
        std::string s = "42";
        buf.append_buf(s);
        // auto buf = create_buf("42");
        // buf.append_buf(s);
        curr_b = std::nullopt;
        decode_buffer<StrCoder<HexNumCoder<int>, ','>, OptCoder<StrCoder<HexNumCoder<int>>>>(buf, callback);
    };

    "bind test"_test = [&]{
        using namespace tasarch::gdb::coders;
        auto callback = [](size_t a, size_t b, size_t c) {

        };
        auto buf = create_buf("42,64");
        // decode_buffer<Str<Hex, ','>, Str<Hex>>(buf, callback);

        using namespace std::placeholders;

        decode_buffer<Str<Hex>, Str<Hex>>(buf, std::bind(callback, 1, _1, _2));
    };
};