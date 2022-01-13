#include <chrono>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>
#include "gdb/asio.h"
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/use_future.hpp>
#include "gdb/common.h"
#include "log/logging.h"
#include "tcp_server_client_test.h"
#include <ut/ut.hpp>
#include "config/config.h"
#include "gdb/bg_executor.h"

namespace tasarch::test::gdb {

    auto create_socket_test(std::function<asio::awaitable<void> (tcp_socket, tcp_socket)> test, std::chrono::milliseconds timeout) -> std::function<void()>
    {
        return [=]{
            auto logger = log::get("test.tcp");
            auto *exec = new tcp_server_client_test();
            exec->timeout = timeout;
            logger->trace("starting tcp_server_client_test");
            exec->start();

            logger->trace("creating socket pair...");
            auto pair = exec->create_pair();
            logger->debug("created socket pair {}, {}", pair.first.local_endpoint(), pair.second.local_endpoint());
            std::future<void> fut = asio::co_spawn(::tasarch::gdb::bg_executor::instance().io_context, [&, timeout]() -> asio::awaitable<void>{
                logger->trace("executing test lambda");
                co_await tasarch::gdb::awaitable_with_timeout(test(std::move(pair.first), std::move(pair.second)), timeout);
                logger->trace("finished executing test lambda");
            }, asio::use_future);
            logger->trace("waiting for future...");
            fut.wait();
            logger->trace("future finished, stopping executor...");
            exec->stop();
            // causes exception to be thrown!
            fut.get();
        };
    }

    auto create_dual_socket_test(std::function<asio::awaitable<void> (tcp_socket)> remote_test, std::function<asio::awaitable<void> (tcp_socket)> local_test, std::chrono::milliseconds timeout) -> std::function<void()>
    {
        return [=]{
            auto logger = log::get("test.tcp");
            auto *exec = new tcp_server_client_test();
            exec->timeout = timeout;
            logger->trace("starting tcp_server_client_test");
            exec->start();

            logger->trace("creating socket pair...");
            auto pair = exec->create_pair();
            logger->debug("created socket pair {}, {}", pair.first.local_endpoint(), pair.second.local_endpoint());
            std::future<void> fut = asio::co_spawn(::tasarch::gdb::bg_executor::instance().io_context, [&, timeout]() -> asio::awaitable<void>{
                logger->trace("executing test lambda");
                co_await tasarch::gdb::awaitable_with_timeout((remote_test(std::move(pair.first)) && local_test(std::move(pair.second))), timeout);
                logger->trace("finished executing test lambda");
            }, asio::use_future);
            logger->trace("waiting for future...");
            fut.wait();
            logger->trace("future finished, stopping executor...");
            exec->stop();
            // causes exception to be thrown!
            // I think because of the && the exception is gonna be messed up :/
            fut.get();
        };
    }

    void tcp_server_client_test::start()
    {
        if (this->acceptor != nullptr) {
            logger->warn("already started!");
            return;
        }
        logger->info("Creating tcp acceptor on port {}", this->port);
        tcp_acceptor acceptor(::tasarch::gdb::bg_executor::instance().io_context, {asio::ip::tcp::v4(), this->port});
        this->acceptor = std::make_shared<tcp_acceptor>(std::move(acceptor));
    }

    void tcp_server_client_test::stop()
    {
        if (this->acceptor == nullptr) {
            logger->warn("Never started!");
            return;
        }

        this->acceptor->close();
        this->acceptor = nullptr;
    }

    auto tcp_server_client_test::create_pair_async() -> asio::awaitable<std::pair<tcp_socket, tcp_socket>>
    {
        std::lock_guard lk(this->mutex);
        tcp_socket local(::tasarch::gdb::bg_executor::instance().io_context);
        logger->debug("Launching client connection coroutine");
        asio::awaitable<void> local_conn = local.async_connect({asio::ip::tcp::v4(), this->port}, asio::use_awaitable);
        logger->debug("Launching server accept coroutine");
        asio::awaitable<tcp_socket> remote_conn = this->acceptor->async_accept();
        logger->debug("Waiting on both to finish");
        tcp_socket remote = co_await (std::move(local_conn) && std::move(remote_conn));
        co_return std::make_pair(std::move(local), std::move(remote));
    }

    auto tcp_server_client_test::create_pair() -> std::pair<tcp_socket, tcp_socket>
    {
        /**
         * @brief We need to do the song and dance with `std::shared_ptr` here, since `asio::co_spawn` wants to be able to default initialize the result, in case of errors.
         * Don't ask me why, that seems kinda like a bad design, but whatever, it's not too bad.
         * 
         */
        std::future<std::pair<std::shared_ptr<tcp_socket>, std::shared_ptr<tcp_socket>>> fut = asio::co_spawn(::tasarch::gdb::bg_executor::instance().io_context, [&]() -> asio::awaitable<std::pair<std::shared_ptr<tcp_socket>, std::shared_ptr<tcp_socket>>>{
            std::pair<tcp_socket, tcp_socket> res = co_await ::tasarch::gdb::awaitable_with_timeout(this->create_pair_async(), this->timeout);
            co_return std::make_pair(std::make_shared<tcp_socket>(std::move(res.first)), std::make_shared<tcp_socket>(std::move(res.second)));
            // co_return std::move(res);
        }, asio::use_future);
        fut.wait();
        auto res = fut.get();
        return std::make_pair(std::move(*res.first), std::move(*res.second));
    }
} // namespace tasarch::test::gdb

namespace ut = boost::ut;
namespace gdb = tasarch::test::gdb;

ut::suite tcp_tests = []{
    using namespace ut;
    using asio::ip::tcp;

    auto config_val = tasarch::config::parse_toml("logging.test.gdb.level = 'trace'\nlogging.bgexec.level = 'trace'");
    tasarch::config::conf()->load_from(config_val);

    "simple tcp test"_test = gdb::create_socket_test([](tcp::socket remote, tcp::socket local) -> asio::awaitable<void>{
        std::string rem_to_loc = "asdf";
        auto res = co_await remote.async_send(asio::buffer(rem_to_loc), asio::use_awaitable);
        std::string rem_msg;
        rem_msg.resize(1024);
        res = co_await local.async_receive(asio::buffer(rem_msg), asio::use_awaitable);
        // rem_msg.resize(res);
        rem_msg.resize(res);
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

    "timeout test"_test = gdb::create_socket_test([](tcp::socket remote, tcp::socket local) -> asio::awaitable<void>{
        std::string rem_to_loc = "asdf";

        // no exception here
        auto res = co_await tasarch::gdb::awaitable_with_timeout(remote.async_send(asio::buffer(rem_to_loc), asio::use_awaitable), 1000ms);

        auto logger = tasarch::log::get("test.gdb");

        logger->info("Closing socket");
        local.close();
        logger->info("Sending request now!");

        throws_async_ex(tasarch::gdb::awaitable_with_timeout(remote.async_send(asio::buffer(rem_to_loc), asio::use_awaitable), 1000ms), tasarch::gdb::timed_out);
        co_return;
    });
};
