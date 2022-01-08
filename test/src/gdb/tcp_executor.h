#ifndef __TCP_EXECUTOR_H
#define __TCP_EXECUTOR_H

#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <utility>
#include "gdb/server.h"
#include "gdb/packet_io.h"
#include "log/logger.h"
#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <asio/execution_context.hpp>
#include <asio/ip/basic_endpoint.hpp>

using namespace std::chrono_literals;

namespace tasarch::test::gdb {
    using asio::ip::tcp;

    auto create_socket_test(std::function<asio::awaitable<void>(tcp::socket, tcp::socket)> test, std::chrono::milliseconds timeout = 5000ms) -> std::function<void()>;

    class tcp_executor : log::WithLogger
    {
    public:
        explicit tcp_executor() : log::WithLogger("test.gdb.exec"), io_context(2) {}
        asio::ip::port_type port = 5555;
        asio::io_context io_context;

        std::chrono::milliseconds timeout = 5000ms;

        void start();
        void run();
        void stop();

        auto create_pair() -> std::pair<tcp::socket, tcp::socket>;
        auto accept_connection(asio::ip::tcp::acceptor& acceptor) -> asio::awaitable<void>;

    private:
        std::shared_ptr<std::thread> io_thread = nullptr;
        std::mutex mutex;
        std::condition_variable accepted;
        std::condition_variable connected;

        bool should_stop = false;
        std::shared_ptr<std::shared_ptr<tcp::socket>> incoming = nullptr;
    };
} // namespace tasarch::test::gdb

#endif /* __TCP_EXECUTOR_H */
