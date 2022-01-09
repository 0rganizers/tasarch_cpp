#ifndef __GDB_COMMON_H
#define __GDB_COMMON_H

/**
 * @file common.h
 * @brief Defines common functions and types used across coroutines and asio stuff in tasarch.
 * @todo Does it make more sense to define in a separate folder & namespace, rather than gdb one?
 */

#include <chrono>
#include <stdexcept>
#include <variant>
#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <asio/deadline_timer.hpp>
#include <asio/basic_waitable_timer.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/use_awaitable.hpp>
#include "bg_executor.h"

using namespace asio::experimental::awaitable_operators;
using tcp_socket = asio::use_awaitable_t<>::as_default_on_t<asio::ip::tcp::socket>;
using tcp_acceptor = asio::use_awaitable_t<>::as_default_on_t<asio::ip::tcp::acceptor>;
using namespace std::chrono_literals;

namespace tasarch::gdb {
    /**
     * @brief Use to await something with a timeout.
     * @todo example
     * 
     * @tparam TResult 
     * @tparam Clock 
     * @param awaitable 
     * @param timeout 
     * @return asio::awaitable<TResult> 
     */
    template<typename TResult>
    auto awaitable_with_timeout(asio::awaitable<TResult> awaitable, const std::chrono::system_clock::duration& timeout) -> asio::awaitable<TResult>
    {
        asio::basic_waitable_timer<std::chrono::system_clock> expiration(bg_executor::instance().io_context, timeout);
        std::variant<TResult, std::monostate> res = co_await (std::move(awaitable) || std::move(expiration.async_wait(asio::use_awaitable)));
        if(TResult* act_res = std::get_if<TResult>(&res)) {
            co_return std::move(*act_res);
        }
        /**
         * @todo timeout exception
         */
        throw std::runtime_error("Had timeout!");
    }
    
} // namespace tasarch::gdb


#endif /* __GDB_COMMON_H */
