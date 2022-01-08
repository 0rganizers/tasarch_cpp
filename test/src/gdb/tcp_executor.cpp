#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_future.hpp>
#include "log/logging.h"
#include "tcp_executor.h"
#include <ut/ut.hpp>
#include "config/config.h"

namespace tasarch::test::gdb {

    auto create_socket_test(std::function<asio::awaitable<void> (tcp::socket, tcp::socket)> test, std::chrono::milliseconds timeout) -> std::function<void()>
    {
        return [&]{
            auto logger = log::get("test.gdb");
            auto *exec = new tcp_executor();
            exec->timeout = timeout;
            logger->info("starting tcp_executor");
            exec->start();

            logger->info("creating socket pair...");
            auto pair = exec->create_pair();
            logger->info("created socket pair {}, {}", pair.first.local_endpoint(), pair.second.local_endpoint());
            std::future<void> fut = asio::co_spawn(exec->io_context, [&]() -> asio::awaitable<void>{
                logger->debug("executing test lambda");
                co_await test(std::move(pair.first), std::move(pair.second));
                logger->debug("finished executing test lambda");
            }, asio::use_future);
            logger->info("waiting for future...");
            fut.wait();
            logger->info("future finished, stopping executor...");
            exec->stop();
        };
    }

    void tcp_executor::start()
    {
        std::lock_guard lock(this->mutex);
        if (this->io_thread != nullptr) {
            logger->warn("Run thread already started!");
            return;
        }

        logger->info("Starting thread for tcp executor io context");
        this->should_stop = false;
        this->io_thread = std::make_shared<std::thread>(&tcp_executor::run, this);
    }

    void tcp_executor::run()
    {
        bool timeout_reached = false;
        try {
            this->logger->info("Restarting io context");
            this->io_context.restart();
            this->logger->info("Spawning accept coroutine");
            asio::ip::tcp::acceptor acceptor(this->io_context, {asio::ip::tcp::v4(), this->port});
            asio::co_spawn(this->io_context,
                this->accept_connection(acceptor),
                asio::detached);

            // TODO: signals?
            this->logger->info("Running io context");
            this->io_context.run_for(this->timeout);
            if (!this->should_stop) { timeout_reached = true; }
            else {
                this->logger->debug("closing tcp acceptor");
                acceptor.close();
            }
            this->logger->info("Finished running io context");
        } catch (std::exception& e) {
            this->logger->error("Had unhandled exception while running tcp executor:\n{}", e.what());
            this->stop();
        }

        if (timeout_reached) {
            this->logger->error("Reached timeout while executing");
            throw std::runtime_error("Reached timeout while executing!");
        }
    }

    void tcp_executor::stop()
    {
        std::lock_guard lock(this->mutex);
        if (this->io_thread == nullptr) {
            logger->warn("Thread was never started!");
            return;
        }
        
        if (this->should_stop) {
            logger->warn("Thread was already told to stop!");
            return;
        }
        
        this->logger->info("Stopping executor...");
        this->should_stop = true;
        this->io_context.stop();
        this->logger->info("Stopped io context, waiting on thread to exit...");
        this->io_thread->join();
        this->logger->info("Thread exited, cleaning up...");
        this->io_thread = nullptr;
    }

    auto tcp_executor::create_pair() -> std::pair<tcp::socket, tcp::socket>
    {
        std::shared_ptr<std::shared_ptr<tcp::socket>> remote = std::make_shared<std::shared_ptr<tcp::socket>>(nullptr);
        std::unique_lock lk(this->mutex);

        this->logger->info("Creating remote, local pair...");

        // Some other thread is currently inside create pair as well!
        if (this->incoming != nullptr) {
            this->logger->debug("other thread is also creating, waiting for it to finish!");
            this->connected.wait(lk, [this]{ return this->incoming == nullptr; });
        }

        this->logger->debug("We are now creating!");

        this->incoming = remote;

        tcp::socket local(this->io_context);
        this->logger->debug("notifying other threads, that we are about to connect!");
        lk.unlock();
        this->connected.notify_all();

        this->logger->info("Trying to connect to server...");

        local.connect({tcp::v4(), this->port});

        this->logger->info("Connected to server! Now we tell the others!");
        lk.lock();
        if (*remote == nullptr) {
            this->logger->warn("Hmm for some reason remote was still null, waiting on signal!");
            this->accepted.wait(lk, [remote]{ return *remote != nullptr; });
        }
        this->logger->debug("Now really notifying others they are good too go again.");
        lk.unlock();
        this->connected.notify_all();

        return std::make_pair(std::move(**remote), std::move(local));
    }

    auto tcp_executor::accept_connection(asio::ip::tcp::acceptor& acceptor) -> asio::awaitable<void>
    {
        this->logger->info("Starting accepting of connections...");
        while (!this->should_stop) {
            auto tcp_conn = co_await acceptor.async_accept(asio::use_awaitable);
            std::unique_lock lk(this->mutex);

            this->connected.wait(lk, [this]{ return this->incoming != nullptr; });

            this->logger->info("Accepted connection from {}", tcp_conn.remote_endpoint());

            std::shared_ptr<tcp::socket> remote = std::make_shared<tcp::socket>(std::move(tcp_conn));
            *this->incoming = remote;

            // notify create_pair, that remote is now available.
            // but first reset incoming, so that we wait for the next create_pair invocation.
            this->incoming = nullptr;
            lk.unlock();
            this->accepted.notify_one();
        }
        this->logger->info("Stopped running, closing acceptor...");
        acceptor.close();
    }
} // namespace tasarch::test::gdb

namespace ut = boost::ut;
namespace gdb = tasarch::test::gdb;

ut::suite tcp_tests = []{
    using namespace ut;
    using asio::ip::tcp;

    auto config_val = tasarch::config::parse_toml("logging.test.gdb.level = 'trace'");
    tasarch::config::conf.load_from(config_val);

    "simple tcp test"_test = gdb::create_socket_test([](tcp::socket remote, tcp::socket local) -> asio::awaitable<void>{
        std::string rem_to_loc = "asdf";
        auto res = co_await remote.async_send(asio::buffer(rem_to_loc), asio::use_awaitable);
        ut::log << "sent return value: " << res;
        std::string rem_msg;
        rem_msg.resize(1024);
        res = co_await local.async_receive(asio::buffer(rem_msg), asio::use_awaitable);
        // rem_msg.resize(res);
        rem_msg.resize(res);
        ut::log << "recv return value: " << res;
        expect(rem_msg == rem_to_loc) << "remote was" << rem_msg << "but expected asdf, size:" << rem_msg.size() << "expected:" << rem_to_loc.size();

        std::string loc_to_rem = "fdsa";
        co_await local.async_send(asio::buffer(loc_to_rem), asio::use_awaitable);
        std::string loc_msg;
        loc_msg.resize(1024);
        res = co_await remote.async_receive(asio::buffer(loc_msg), asio::use_awaitable);
        loc_msg.resize(res);
        expect(loc_msg == loc_to_rem) << "local was" << loc_msg << "but expected fdsa";
        co_return;
    });
};
