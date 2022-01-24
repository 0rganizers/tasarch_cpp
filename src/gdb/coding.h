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
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <tuple>
#include <concepts>
#include <type_traits>
#include <fmt/core.h>
#include <charconv>
#include <vector>
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

    /**
     * @brief Describes a type that implements a specific encoding, for example, encoding an array of bytes to a hex string.
     * 
     * @tparam TEncoder The type implementing the function for encoding.
     * @tparam TValue The value that will be encoded, in our example `std::vector<u8>`.
     * @tparam TToLocation The "location" of where to encode to. Made variable here, to support encoding to a `buffer` or `std::string`.
     @todo In the future, it would be cool to not have to rely on the location being specific to an encoder, but rather variable.
     */
    template<typename TEncoder, typename TValue, typename TToLocation>
    concept Encoding = requires(TValue& val, TToLocation& buf) {
        {TEncoder::encode_to(val, buf)};
    };

    /**
     * @brief Describes a type that implements a specific decoding, for example, decoding a hex string to an array of bytes.
     * 
     * @tparam TDecoder The type implementing the function for decoding.
     * @tparam TValue The type of value that will be decoded, in our example `std::vector<u8>`.
     * @tparam TFromLocation The "location" of where to decode from. Made variable here, to support encoding to a `buffer` or `std::string`.
     @todo The same as for `Encoding`, the location should not be a template argument!
     */
    template<typename TDecoder, typename TValue, typename TFromLocation>
    concept Decoding = requires(TFromLocation& buf) {
        {TDecoder::decode_from(buf)} -> convertible_to<TValue>;
    };

    /**
     * @brief Combines `Encoding` and `Decoding` to a single `Coding` concept. This allows us to require both.
     * 
     * @tparam TCoder 
     */
    template<typename TCoder>
    concept Coding = Encoding<TCoder, typename TCoder::value_type, typename TCoder::loc_type>
                        and Decoding<TCoder, typename TCoder::value_type, typename TCoder::loc_type>
                        and requires {
                            typename TCoder::value_type;
                            typename TCoder::loc_type;
                        };

    /**
     * @name Black Magic
     * The stuff below is used to provide a generic function, that can decode a sequence of values, then call a callback with the decoded values as arguments.
     * It uses some C++ template black magic to achieve this.
     */
    /// @{
#pragma mark Black Magic

    /**
     * @brief Anything but a coder. Probably not actually useful anymore, thought it was needed for some of the black magic lul.
     * 
     * @tparam T 
     */
    template<typename T>
    concept NotCoding = not Coding<T>;

    /**
     * @brief Describes a callback that can be called with the given arguments.
     * 
     * @tparam TCallback 
     * @tparam TArgs The arguments of the callback.
     */
    template<typename TCallback, typename ...TArgs>
    concept Callback = std::is_invocable<TCallback, TArgs...>::value;

    /**
     * @brief Helper thingy, to get the `value_type` from a `Coding` template argument.
     * 
     * @tparam TCoder 
     */
    template<tasarch::gdb::Coding TCoder>
    using extract_arg = typename TCoder::value_type;

    /**
     * @brief Old implementation of the whole decode with callback thingy. Not used anymore.
     * 
     * @tparam TArgs 
     * @tparam TCallback 
     * @param buf 
     * @param callback 
     * @param args 
     */
    template<typename ...TArgs, Callback<TArgs...> TCallback>
    void decode_buffer_impl(tasarch::gdb::buffer& buf, TCallback callback, TArgs... args)
    {
        // printf("Hello there: %s %c %c", args...);
        callback(args...);
    }

    /**
     * @brief Old implementation of the whole decode with callback thingy. Not used anymore.
     * 
     */
    template<tasarch::gdb::Coding TCoder, tasarch::gdb::Coding ...TCoders, NotCoding ...TArgs, Callback<TArgs..., extract_arg<TCoder>, extract_arg<TCoders>...> TCallback>
        // requires ()
                    //or Callback<TCallback, TArgs..., extract_arg<TCoder>>)
    void decode_buffer_impl(tasarch::gdb::buffer& buf, TCallback callback, TArgs... args)
    {
        auto val = TCoder::decode_from(buf);
        if (buf.read_size() < 1) {
            callback(args..., val);
        } else {
            decode_buffer_impl<TCoders...>(buf, callback, args..., val);
        }
        // decode_buffer_impl<TCoders...>(buf, callback, args..., TCoder::decode_from(buf));
    }

    /**
     * @brief Old implementation of the whole decode with callback thingy. Not used anymore.
     * 
     */
    template<tasarch::gdb::Coding ...TCoders, Callback<extract_arg<TCoders>...> TCallback>
    void decode_buffer_old(tasarch::gdb::buffer& buf, TCallback callback)
    {
        decode_buffer_impl<TCoders...>(buf, callback);
    }

    /**
     * @brief Magic method, that removes some boilerplate for parsing stuff from a GDB connection.
     * In essence, you describe the sequence of arguments and how they are decoded in the template arguments and provide a corresponding callback function.
     * This function will then take care of the rest.
     *
     * @m_class{m-block m-success}
     * @par Example
     * Let's imagine, you have some GDB command with the following "signature": `some_num,some_str[,some_hex_encoded_bytes]`.
     * You would then use this function as follows (see `tasarch::gdb::coders` for details on how the template arguments work):
     * @code {.cpp}
     auto cb = [](size_t a, std::string b, std::vector<u8> c){
         // Callback stuff here
     };
     using namespace tasarch::gdb::coders;
     decode_buffer<Str<Hex, ',', true>, Str<Id, ','>, Opt<Bytes<>>>(some_buf, cb);
     * @endcode
     * 
     * 
     * @tparam TCoders 
     * @tparam TCallback 
     * @param buf 
     * @param callback 
     */
    template<tasarch::gdb::Coding ...TCoders, Callback<extract_arg<TCoders>...> TCallback>
    void decode_buffer(tasarch::gdb::buffer& buf, TCallback callback)
    {
        std::apply(callback, std::tuple{TCoders::decode_from(buf)...});
    }

    ///@}

    /**
     * @name Coders
     * Below we define some commonly used `Coding`s, that are used throughout the GDB protocol.
     */
#pragma mark Coders

    /**
     * @brief Like id function, does nothing to its input.
     * Useful as default cases for certain coders that have a "child".
     * 
     * @tparam TLocation 
     */
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

    /**
     * @brief Decodes the contents of buffer up until it can find the separator `Sep` or the end of the buffer is reached.
     * If `SepReq` is true, an exception is thrown, if the end of string is reached before finding the separator.
     * Once it has a string of characters, it calls upon the underlying child coder to decode the string into a useful value.
     * This allows nicely nesting coders, to e.g. decode a hex number separated by a comma.
     * If you need to have the full string returned, you can use the `IdCoder` as mentioned before.
     * @note This alone does not help you implement optional arguments, but it can help.
     * 
     * @tparam TChildCoder 
     * @tparam Sep 
     * @tparam SepReq 
     */
    template<Coding TChildCoder = IdCoder<std::string>, char Sep = '\0', bool SepReq = false>
    struct StrCoder
    {
        using value_type = typename TChildCoder::value_type;
        using loc_type = buffer;

        static auto decode_from(loc_type& buf) -> value_type
        {
            std::string ret;
            u8 curr = 0;
            while (true) {
                if (buf.read_size() < 1) {
                    if (SepReq) {
                        // crash if this is uncommented
                        // std::string error_msg = fmt::format("Unexpected end of string, expected to find {:c} first!", Sep);
                        throw std::runtime_error("Unexpected end of string!");
                    }
                    break;
                }
                curr = buf.get_byte();
                
                if (curr == Sep) { break; }

                ret += curr;
            }

            return TChildCoder::decode_from(ret);
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
            std::string ret;
            TChildCoder::encode_to(val,  ret);
            buf.append_buf(ret);
        }
    };

    static_assert(Coding<StrCoder<>>, "Expected StrCoder to conform to coding!");

    template<Coding TChildCoder>
    struct OptCoder
    {
        using value_type = std::optional<typename TChildCoder::value_type>;
        using loc_type = buffer;

        static auto decode_from(loc_type& buf) -> value_type
        {
            if (buf.read_size() < 1) {
                return std::nullopt;
            }

            return TChildCoder::decode_from(buf);
        }

        static auto encode_to(value_type& value, loc_type& buf)
        {
            if (value == std::nullopt) {
                return;
            }

            return TChildCoder::encode_to(value.value(),  buf);
        }
    };

    static_assert(Coding<OptCoder<StrCoder<>>>, "Expected OptCoder to conform to coding!");

    template<integral T, size_t Base = 0>
    struct NumCoder
    {
        using value_type = T;
        using loc_type = std::string;

        static auto decode_from(loc_type& buf) -> value_type
        {
            value_type ret;
            auto [ptr, error] = std::from_chars(buf.data(), buf.data() + buf.size(), ret, Base);
            if (error != std::errc()) {
                throw std::system_error(std::make_error_code(error));
            }
            return ret;
        }

        static auto encode_to(value_type& val, loc_type& buf)
        {
            buf.resize(17);
            
            auto [ptr, error] = std::to_chars(buf.data(), buf.data() + static_cast<size_t>(buf.size()), val, Base);
             if (error != std::errc()) {
                throw std::system_error(std::make_error_code(error));
            }
            size_t diff = ptr - buf.data();
            buf.resize(diff);
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

    template<Coding TChildCoder = IdCoder<std::string>, char Sep = ';'>
    struct ArrayCoder
    {
        using value_type = std::vector<typename TChildCoder::value_type>;
        using loc_type = buffer;
        using elem_type = typename TChildCoder::value_type;
        using str_coder = StrCoder<TChildCoder, Sep>;
        using size_type = typename value_type::size_type;

        static auto decode_from(loc_type& buf) -> value_type
        {
            value_type ret;
            while (buf.read_size() > 0)
            {
                auto elem = str_coder::decode_from(buf);
                ret.push_back(elem);
            }
            return ret;
        }

        static auto encode_to(value_type& val, loc_type& buf)
        {
            for (size_type i = 0; i < val.size(); i++) {
                auto elem = val.at(i);
                str_coder::encode_to(elem, buf);
                if (i != val.size() - 1)
                {
                    buf.put_byte(Sep);
                }
            }
        }
    };

    static_assert(Coding<ArrayCoder<HexNumCoder<size_t>>>, "Expected ArrayCoder to conform to coding!");

    namespace coders {
        using Hex = HexNumCoder<size_t>;

        template<typename TLocation = std::string>
        using Id = IdCoder<TLocation>;
        
        template<Coding TChildCoder = IdCoder<std::string>, char Sep = '\0', bool SepReq = false>
        using Str = StrCoder<TChildCoder, Sep, SepReq>;

        template<MyContainer TContainer = std::vector<u8>>
        using Bytes = BytesCoder<TContainer>;

        template<Coding TChildCoder>
        using Opt = OptCoder<TChildCoder>;

        template<Coding TChildCoder, char Sep = ';'>
        using Array = ArrayCoder<TChildCoder, Sep>;
    } // namespace coders
} // namespace tasarch::gdb

#endif /* __GDB_CODING_H */