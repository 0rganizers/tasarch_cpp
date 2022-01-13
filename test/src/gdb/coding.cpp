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

template<typename TNotCoder>
concept NotCoding = not tasarch::gdb::Coding<TNotCoder>;

template<typename TCallback, typename ...TArgs>
concept Callback = std::is_invocable<TCallback, TArgs...>::value;

template<tasarch::gdb::Coding TCoder>
using extract_arg = typename TCoder::value_type;

/// no decoding left, callback!
template<typename ...TArgs, Callback<TArgs...> TCallback>
void test_generic_encoder_impl(tasarch::gdb::buffer& buf, TCallback callback, TArgs... args)
{
    // printf("Hello there: %s %c %c", args...);
    callback(args...);
}

template<tasarch::gdb::Coding TCoder, tasarch::gdb::Coding ...TCoders, NotCoding ...TArgs, Callback<TArgs..., extract_arg<TCoder>, extract_arg<TCoders>...> TCallback>
void test_generic_encoder_impl(tasarch::gdb::buffer& buf, TCallback callback, TArgs... args)
{
    test_generic_encoder_impl<TCoders...>(buf, callback, args..., TCoder::decode_from(buf));
}

template<tasarch::gdb::Coding ...TCoders, Callback<extract_arg<TCoders>...> TCallback>
void test_generic_encoder(tasarch::gdb::buffer& buf, TCallback callback)
{
    test_generic_encoder_impl<TCoders...>(buf, callback);
}

template<tasarch::gdb::Coding ...TCoders, Callback<extract_arg<TCoders>...> TCallback>
void test_generic_encoder2(tasarch::gdb::buffer& buf, TCallback callback)
{
    std::apply(callback, std::tuple{TCoders::decode_from(buf)...});
}

struct test_char_dec
{
    using value_type = u8;
    using loc_type = tasarch::gdb::buffer;

    static auto decode_from(tasarch::gdb::buffer& buf) -> value_type
    {
        return buf.get_byte();
    }

    static auto encode_to(value_type val, tasarch::gdb::buffer& buf)
    {
        
    }
};

static_assert(tasarch::gdb::Coding<test_char_dec>, "test_char_dec should conform to Coding!");

template<char Sep>
struct test_str_dec
{
    using value_type = std::string;
    using loc_type = tasarch::gdb::buffer;

    static auto decode_from(tasarch::gdb::buffer& buf) -> value_type
    {
        std::string ret;
        u8 curr = 0;
        while (buf.read_size() > 0 && ((curr = buf.get_byte()) != Sep)) {
            ret += curr;
        }

        return ret;
    }

    static auto encode_to(value_type val, tasarch::gdb::buffer& buf)
    {

    }
};

using test_str_dec_komma = test_str_dec<','>;

static_assert(tasarch::gdb::Coding<test_str_dec_komma>, "test_char_dec should conform to Coding!");

ut::suite coding_tests = []{
    using namespace ut;
    using asio::ip::tcp;
    using namespace tasarch::gdb;

    auto config_val = tasarch::config::parse_toml("logging.test.gdb.level = 'trace'\nlogging.gdb.level = 'trace'");
    tasarch::config::conf()->load_from(config_val);
    auto logger = tasarch::log::get("test.gdb");

    "test generic encoder"_test = [&]{
        std::string test = "testing";
        auto callback = [&](std::string a, u8 b, u8 c){
            logger->info("LMFAO CALLBACK CALLED: {}, {}, {}", a, b, c);
            expect(a == test);
            expect(b == 'f');
            expect(c == 'e');
        };
        // check that its working as is.
        callback(test, 'f', 'e');
        // now do the magic encoding :)
        std::string input = test + "," + "f" + "e";
        buffer my_buf(100);
        my_buf.append_buf(input);
        test_generic_encoder<test_str_dec_komma, test_char_dec, test_char_dec>(my_buf, callback);
        my_buf.reset();
        my_buf.append_buf(input);
        test_generic_encoder2<test_str_dec_komma, test_char_dec, test_char_dec>(my_buf, callback);
        // test_generic_encoder<test_str_dec_komma, test_char_dec, test_char_dec>(my_buf);
    };
};