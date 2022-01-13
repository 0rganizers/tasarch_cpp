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
#include "buffer.h"

namespace tasarch::gdb {
    template<typename TEncoder, typename TValue, typename TToLocation>
    concept Encoding = requires(TValue val, TToLocation& buf) {
        {TEncoder::encode_to(val, buf)};
    };

    template<typename TDecoder, typename TValue, typename TFromLocation>
    concept Decoding = requires(TFromLocation& buf) {
        {TDecoder::decode_from(buf)} -> convertible_to<TValue>;
    };

    template<typename TCoder>
    concept Coding = Encoding<TCoder, typename TCoder::value_type, typename TCoder::loc_type> and Decoding<TCoder, typename TCoder::value_type, typename TCoder::loc_type>;

#pragma mark Black Magic
    /**
     * @todo not sure this is actually needed.
     * 
     * @tparam TNotCoder 
     */
    template<typename TNotCoder>
    concept NotCoding = not tasarch::gdb::Coding<TNotCoder>;

    template<typename TCallback, typename ...TArgs>
    concept Callback = std::is_invocable<TCallback, TArgs...>::value;

    template<tasarch::gdb::Coding TCoder>
    using extract_arg = typename TCoder::value_type;

    /// no decoding left, callback!
    template<typename ...TArgs, Callback<TArgs...> TCallback>
    void decode_buffer_impl(tasarch::gdb::buffer& buf, TCallback callback, TArgs... args)
    {
        callback(args...);
    }

    template<tasarch::gdb::Coding TCoder, tasarch::gdb::Coding ...TCoders, NotCoding ...TArgs, Callback<TArgs..., extract_arg<TCoder>, extract_arg<TCoders>...> TCallback>
    void decode_buffer_impl(tasarch::gdb::buffer& buf, TCallback callback, TArgs... args)
    {
        decode_buffer_impl<TCoders...>(buf, callback, args..., TCoder::decode_from(buf));
    }

    template<tasarch::gdb::Coding ...TCoders, Callback<extract_arg<TCoders>...> TCallback>
    void decode_buffer(tasarch::gdb::buffer& buf, TCallback callback)
    {
        decode_buffer_impl<TCoders...>(buf, callback);
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

        static auto encode_to(value_type value, loc_type& buf)
        {
            buf = value;
        }
    };

    template<Coding TChildCoder = IdCoder<std::string>, char Sep = '\0'>
    struct StrCoder
    {
        using value_type = std::string;
        using loc_type = buffer;

        static auto decode_from(loc_type& buf) -> value_type
        {
            std::string ret;
            u8 curr = 0;
            while (buf.read_size() > 0 && ((curr = buf.get_byte()) != Sep)) {
                ret += curr;
            }

            return TChildCoder::decode_from(curr);
        }

        static auto encode_to(value_type val, loc_type& buf)
        {
            TChildCoder::encode_to(val,  val);
            buf.append_buf(val);
        }
    };

    static_assert(Coding<StrCoder<>>, "Expected StrCoder to conform to coding!");
} // namespace tasarch::gdb

#endif /* __GDB_CODING_H */