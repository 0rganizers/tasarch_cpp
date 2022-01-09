#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/use_future.hpp>
#include "log/logging.h"
#include "tcp_executor.h"
#include <ut/ut.hpp>
#include "config/config.h"
#include "gdb/bg_executor.h"

namespace tasarch::test::gdb {

    auto create_socket_test(std::function<asio::awaitable<void> (tcp_socket, tcp_socket)> test, std::chrono::milliseconds timeout) -> std::function<void()>
    {
        return [&]{
            auto logger = log::get("test.tcp");
            auto *exec = new tcp_server_client_test();
            exec->timeout = timeout;
            logger->trace("starting tcp_server_client_test");
            exec->start();

            logger->trace("creating socket pair...");
            auto pair = exec->create_pair();
            logger->debug("created socket pair {}, {}", pair.first.local_endpoint(), pair.second.local_endpoint());
            std::future<void> fut = asio::co_spawn(::tasarch::gdb::bg_executor::instance().io_context, [&]() -> asio::awaitable<void>{
                logger->trace("executing test lambda");
                co_await test(std::move(pair.first), std::move(pair.second));
                logger->trace("finished executing test lambda");
            }, asio::use_future);
            logger->trace("waiting for future...");
            fut.wait();
            logger->trace("future finished, stopping executor...");
            exec->stop();
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
        std::future<std::pair<std::shared_ptr<tcp_socket>, std::shared_ptr<tcp_socket>>> fut = asio::co_spawn(::tasarch::gdb::bg_executor::instance().io_context, [&]() -> asio::awaitable<std::pair<std::shared_ptr<tcp_socket>, std::shared_ptr<tcp_socket>>>{
            // asio::basic_waitable_timer<std::chrono::system_clock> expiration(tasarch::gdb::bg_executor::instance().io_context, timeout);
            // std::pair<tcp_socket, tcp_socket> res = co_await this->create_pair_async();
            // co_return std::make_pair(std::move(res.first), std::move(res.second));;
            // std::variant<std::pair<tcp_socket, tcp_socket>, std::monostate> res = co_await this->create_pair_async();// || expiration.async_wait(asio::use_awaitable));
            // std::pair<tcp_socket, tcp_socket> act_res = std::get<std::pair<tcp_socket, tcp_socket>>(std::move(res));
            // co_return std::move(act_res);
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
