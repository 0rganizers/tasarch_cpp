#include <memory>
#include <mutex>
#include "server.h"  
#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/use_awaitable.hpp>
	
namespace tasarch::gdb {
    void server::start()
    {
        std::lock_guard lock(run_mutex);
        if (this->io_thread != nullptr) {
            logger->warn("Run thread already started!");
            return;
        }

        logger->info("Starting thread for gdb server io context");
        this->should_stop = false;
        this->io_thread = std::make_shared<std::thread>(&server::run, this);
    }

    void server::run()
    {
        try {
            this->logger->info("Restarting io context");
            this->io_context.restart();
            this->logger->info("Spawning accept coroutine");
            asio::co_spawn(this->io_context,
                this->accept_connection(asio::ip::tcp::acceptor(this->io_context, {asio::ip::tcp::v4(), this->port})),
                asio::detached);

            // TODO: signals?
            this->logger->info("Running io context");
            this->io_context.run();
            this->logger->info("Finished running io context");
        } catch (std::exception& e) {
            this->logger->error("Had unhandled exception while running gdbserver:\n{}", e.what());
        }
    }

    void server::stop()
    {
        std::lock_guard lock(run_mutex);
        if (this->io_thread == nullptr) {
            logger->warn("Thread was never started!");
            return;
        }

        if (this->should_stop) {
            logger->warn("Thread was already told to stop!");
            return;
        }
        
        this->logger->info("Stopping gdb connections...");

        for (auto& conn : this->connections) {
            conn->stop();
        }
        
        this->logger->info("Stopping gdb server...");
        this->should_stop = true;
        this->io_context.stop();
        this->logger->info("Stopped io context, waiting on thread to exit...");
        this->io_thread->join();
        this->logger->info("Thread exited, cleaning up...");
        this->io_thread = nullptr;
    }

    auto server::accept_connection(asio::ip::tcp::acceptor acceptor) -> asio::awaitable<void>
    {
        this->logger->info("Starting accepting of connections...");
        while (!this->should_stop) {
            auto tcp_conn = co_await acceptor.async_accept(asio::use_awaitable);
            this->logger->info("Accepted connection from {}", tcp_conn.remote_endpoint());

            // TODO: lock here!
            auto conn = std::make_shared<connection>(std::move(tcp_conn), this->debugger);
            this->connections.push_back(conn);
            conn->start();
        }
    }
} // namespace tasarch::gdb