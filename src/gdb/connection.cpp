#include <exception>
#include "connection.h"  
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
	
namespace tasarch::gdb {
    void connection::start()
    {
        if (this->running) {
            this->logger->warn("Already running!");
            return;
        }
        this->running = true;
        this->should_stop = false;
        asio::co_spawn(socket.get_executor(), [self = shared_from_this()]{ return self->process(); }, asio::detached);
    }

    void connection::stop()
    {
        /**
         * @todo Locking!
         */
        if (!this->running) {
            this->logger->warn("Not running!");
            return;
        }
        if (this->should_stop) {
            this->logger->warn("Already stopped!");
            return;
        }
        this->should_stop = true;
    }

    auto connection::process() -> asio::awaitable<void>
    {
        this->logger->info("Starting processing loop...");
        try {
            while (true) {
                if (!socket.is_open()) {
                    this->logger->info("Remote socket {} closed, exiting...", socket.remote_endpoint());
                    break;
                }
                if (!should_stop) {
                    this->logger->info("Got stop signal, exiting...");
                    break;
                }
            }
        } catch (std::exception& e) {
            this->logger->error("Unhandled exception in processing loop: {}", e.what());
            this->stop();
        }
    }
} // namespace tasarch::gdb