#include <memory>
#include <mutex>
#include "server.h"  
#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/use_awaitable.hpp>
#include "bg_executor.h"

namespace tasarch::gdb {
    void server::start()
    {
        std::lock_guard lock(run_mutex);
        if (this->running) {
            logger->warn("Server already started!");
        }
        this->running = true;
        logger->info("Starting gdbstub on port {}", this->port);
        asio::co_spawn(bg_executor::instance().io_context,
                this->accept_connection(asio::ip::tcp::acceptor(bg_executor::instance().io_context, {asio::ip::tcp::v4(), this->port})),
                asio::detached);
    }

    void server::stop()
    {
        std::lock_guard lock(run_mutex);
        if (!this->running) {
            logger->warn("Server was never started!");
            return;
        }

        this->logger->info("Stopping gdb connections...");

        for (auto& conn : this->connections) {
            conn->stop();
        }
        this->connections.clear();
        
        this->running = false;
    }

    auto server::accept_connection(asio::ip::tcp::acceptor acceptor) -> asio::awaitable<void>
    {
        this->logger->info("Starting accepting of connections...");
        while (this->running) {
            /**
             * @todo Cancellation slot here!
             */
            auto tcp_conn = co_await acceptor.async_accept(asio::use_awaitable);
            this->logger->info("Accepted connection from {}", tcp_conn.remote_endpoint());

            {
                std::lock_guard lk(run_mutex);
                auto conn = std::make_shared<connection>(std::move(tcp_conn), this->debugger);
                this->connections.push_back(std::shared_ptr<connection>(conn));
                conn->start();
            }
        }
    }
} // namespace tasarch::gdb
