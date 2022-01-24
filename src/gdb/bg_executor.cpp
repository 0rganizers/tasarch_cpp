#include <chrono>
#include <memory>
#include <stdexcept>
#include <asio/awaitable.hpp>
#include <asio/basic_waitable_timer.hpp>
#include <asio/detached.hpp>
#include <asio/use_awaitable.hpp>
#include "bg_executor.h"

using namespace std::chrono_literals;
	
namespace tasarch::gdb {
    void bg_executor::start()
    {
        if (this->running) {
            logger->warn("Already running!");
            return;
        }
        this->running = true;
        this->created_threads = false;
        this->should_stop = false;

        this->io_context.restart();

        // we need to have some idle coroutine running, otherwise io_context will immediately exit :/
        asio::co_spawn(this->io_context, this->idle(), asio::detached);

        logger->info("Starting {} threads for handling io + coroutines...", this->num_threads);
        if (this->num_threads < 1) {
            throw std::runtime_error("Cannot run bg_executor with less than one thread");
        }

        for (int i = 0; i < this->num_threads; i++) {
            auto thread = std::make_shared<std::thread>(&bg_executor::run, this);
            this->io_threads.push_back(thread);
        }

        // Wait with enabling running, till all threads are started!
        this->created_threads = true;
    }

    void bg_executor::run()
    {
        this->logger->info("Waiting for all threads to be started...");
        while (!this->created_threads) {
            
        }
        try {
            this->logger->info("Running io context now");
            this->io_context.run();
            this->logger->info("Finished running io context");
        } catch (std::exception& e) {
            this->logger->error("Had unhandled exception while running io operations, stopping now:\n{}", e.what());
            this->stop();
        }
    }

    auto bg_executor::idle() -> asio::awaitable<void>
    {
        asio::basic_waitable_timer<std::chrono::system_clock> timer(this->io_context, 100000h);
        while (this->running) {
            co_await timer.async_wait(asio::use_awaitable);
        }
    }

    void bg_executor::stop()
    {
        if (!this->running) {
            this->logger->warn("Not actually running!");
            return;
        }
        this->running = false;

        /**
         * @todo Cleanup handlers or something here?? Can we do that with cancellation??
         */
        this->logger->info("Stopping io context...");
        this->io_context.stop();
        this->logger->info("Waiting on threads to exit...");
        for (auto& thread : this->io_threads) {
            thread->join();
        }
        this->io_threads.clear();
        this->logger->info("All threads exited!");
    }

    auto bg_executor::instance() -> bg_executor&
    {
        static bg_executor instance;
        return instance;
    }
} // namespace tasarch::gdb