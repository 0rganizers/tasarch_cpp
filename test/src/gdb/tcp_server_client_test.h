#ifndef __TCP_EXECUTOR_H
#define __TCP_EXECUTOR_H

#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <utility>
#include <vector>
#include "gdb/server.h"
#include "gdb/packet_io.h"
#include "log/logger.h"
#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <asio/execution_context.hpp>
#include <asio/ip/basic_endpoint.hpp>
#include "gdb/common.h"
#include "async_test.h"

using namespace std::chrono_literals;

namespace tasarch::test::gdb {

    auto create_socket_test(std::function<asio::awaitable<void>(tcp_socket, tcp_socket)> test, std::chrono::milliseconds timeout = 5000ms) -> std::function<void()>;
    auto create_dual_socket_test(std::function<asio::awaitable<void>(tcp_socket)> remote_test, std::function<asio::awaitable<void>(tcp_socket)> local_test, std::chrono::milliseconds timeout = 5000ms)  -> std::function<void()>;

    class tcp_server_client_test : log::WithLogger
    {
    public:
        explicit tcp_server_client_test() : log::WithLogger("test.tcp") {}

        asio::ip::port_type port = 5555;

        std::chrono::milliseconds timeout = 5000ms;

        void start();
        void stop();

        auto create_pair() -> std::pair<tcp_socket, tcp_socket>;
        auto create_pair_async() -> asio::awaitable<std::pair<tcp_socket, tcp_socket>>;

    private:
        std::shared_ptr<tcp_acceptor> acceptor;
        std::mutex mutex;
    };
} // namespace tasarch::test::gdb

#endif /* __TCP_EXECUTOR_H */
