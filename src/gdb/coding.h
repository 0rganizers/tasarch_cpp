/**
 * @file coding.h
 * @author Leonardo Galli (leo@orgnz.rs)
 * @brief This file contains all helpers used for encoding / decoding command arguments.
 * @version 0.1
 * @date 2022-01-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef __GDB_CODING_H
#define __GDB_CODING_H

#include <functional>
#include <stdexcept>
#include <tuple>
#include <concepts>
#include <type_traits>
#include <fmt/core.h>
#include <charconv>
#include "buffer.h"
#include "util/concepts.h"

namespace tasarch::gdb {

    /**
	 * @brief Simple hex decoding of a nibble.
	 *
	 * @todo Borrowed from atmosphere.
	 * 
	 * @param c The character.
	 * @return int Either the decoded nibble (`[0, 15]`) or `-1` on failure.
	 */
	constexpr auto decode_hex(u8 c) -> int {
		if ('a' <= c && c <= 'f') {
			return 10 + (c - 'a');
		} else if ('A' <= c && c <= 'F') {
			return 10 + (c - 'A');
		} else if ('0' <= c && c <= '9') {
			return 0 + (c - '0');
		} else {
			return -1;
		}
	}

	/**
	 * @brief Simple hex encoding of a nibble.
	 *
	 * @todo Borrowed from atmosphere.
	 * 
	 * @param v The value.
	 * @note Even values beyond `[0, 15]` will be encoded, `v` is just `& 0xf`'d.
	 * @return char 
	 */
	constexpr auto encode_hex(u8 v) -> char {
		return "0123456789abcdef"[v & 0xF];
	}

    template<typename TEncoder, typename TValue, typename TToLocation>
    concept Encoding = requires(TValue& val, TToLocation& buf) {
        {TEncoder::encode_to(val, buf)};
    };

    template<typename TDecoder, typename TValue, typename TFromLocation>
    concept Decoding = requires(TFromLocation& buf) {
        {TDecoder::decode_from(buf)} -> convertible_to<TValue>;
    };

    template<typename TCoder>
    concept Coding = Encoding<TCoder, typename TCoder::value_type, typename TCoder::loc_type> and Decoding<TCoder, typename TCoder::value_type, typename TCoder::loc_type>;

#pragma mark Black Magic
    template<typename TCallback, typename ...TArgs>
    concept Callback = std::is_invocable<TCallback, TArgs...>::value;

    template<tasarch::gdb::Coding TCoder>
    using extract_arg = typename TCoder::value_type;

    /**
        @todo this is probably needed for separator thingies right?
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
    */

    template<tasarch::gdb::Coding ...TCoders, Callback<extract_arg<TCoders>...> TCallback>
    void decode_buffer(tasarch::gdb::buffer& buf, TCallback callback)
    {
        std::apply(callback, std::tuple{TCoders::decode_from(buf)...});
    }

#pragma mark Coders

    template<typename TLocation>
    struct IdCoder
    {
        using value_type = TLocation;
        using loc_type = TLocation;

        static auto decode_from(loc_type& buf) -> value_type
        {
            return buf;
        }

        static auto encode_to(value_type& value, loc_type& buf)
        {
            buf = value;
        }
    };

    template<Coding TChildCoder = IdCoder<std::string>, char Sep = '\0', bool SepReq = false>
    struct StrCoder
    {
        using value_type = std::string;
        using loc_type = buffer;

        static auto decode_from(loc_type& buf) -> value_type
        {
            std::string ret;
            u8 curr = 0;
            while (true) {
                if (buf.read_size() < 1) {
                    if (SepReq) {
                        throw std::runtime_error(fmt::format("Unexpected end of string, expected to find {:c} first!", Sep));
                    }
                    break;
                }
                curr = buf.get_byte();
                
                if (curr == Sep) { break; }

                ret += curr;
            }

            return TChildCoder::decode_from(curr);
        }

        /**
         * @brief 
         * @todo Make separator work here as well!
         * @param val 
         * @param buf 
         * @return auto 
         */
        static auto encode_to(value_type& val, loc_type& buf)
        {
            TChildCoder::encode_to(val,  val);
            buf.append_buf(val);
        }
    };

    static_assert(Coding<StrCoder<>>, "Expected StrCoder to conform to coding!");

    template<integral T, size_t Base = 0>
    struct NumCoder
    {
        using value_type = T;
        using loc_type = std::string;

        static auto decode_from(loc_type& buf) -> value_type
        {
            value_type ret;
            auto [ptr, error] = std::from_chars(buf.data(), buf.data() + buf.size(), ret, Base);
            return ret;
        }

        static auto encode_to(value_type& val, loc_type& buf)
        {
            buf.reserve(16);
            std::to_chars(buf.data(), buf.data() + buf.max_size(), val, Base);
        }
    };

    static_assert(Coding<NumCoder<size_t>>, "Expected NumCoder to conform to coding!");

    template<integral T = size_t>
    using HexNumCoder = NumCoder<T, 16>;
    
    static_assert(Coding<HexNumCoder<>>, "Expected HexNumCoder to conform to coding!");

    template<MyContainer TContainer>
    struct BytesCoder
    {
        using value_type = TContainer;
        using loc_type = buffer;
        using elem_type = typename TContainer::value_type;
        const static size_t elem_size = sizeof(elem_type);
        using size_type = typename TContainer::size_type;

        /**
         * @brief 
         * @todo HEX ERROR CHECKING!
         * @param buf 
         * @return value_type 
         */
        static auto decode_from(loc_type& buf) -> value_type
        {
            value_type ret;
            while (buf.read_size() > 0) {
                elem_type elem;
                u8* ptr = reinterpret_cast<u8*>(&elem);
                for (size_t i = 0; i < elem_size; i++) {
                    u8 hi = buf.get_byte();
                    u8 lo = buf.get_byte();
                    u8 bval = (decode_hex(hi) << 4) | decode_hex(lo);
                    *ptr = bval;
                    ptr++;
                }
                ret.push_back(elem);
            }
            return ret;
        }

        static auto encode_to(value_type& val, loc_type& buf)
        {
            for (size_type i = 0; i < val.size(); i++) {
                elem_type elem = val.at(i);
                u8* ptr = reinterpret_cast<u8*>(&elem);
                for (size_t i = 0; i < elem_size; i++) {
                    u8 bval = *ptr;
                    ptr++;
                    u8 hi = bval >> 4;
                    u8 lo = bval & 0xf;
                    buf.put_byte(encode_hex(hi));
                    buf.put_byte(encode_hex(lo));
                }
            }
        }
    };

    static_assert(Coding<BytesCoder<std::vector<u8>>>, "Expected BytesCoder to conform to coding!");
} // namespace tasarch::gdb

#endif /* __GDB_CODING_H */