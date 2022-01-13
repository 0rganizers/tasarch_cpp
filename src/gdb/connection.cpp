#include <exception>
#include <vector>
#include "connection.h"
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <fmt/core.h>
	
namespace tasarch::gdb {
    void connection::start()
    {
        if (this->running) {
            this->logger->warn("Already running!");
            return;
        }
        this->running = true;
        this->should_stop = false;
        asio::co_spawn(this->packet_io.socket.get_executor(), this->process(), asio::detached);
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
                if (did_break) {
                    this->logger->info("Remote requested a break!");
                    //this->debugger->request_break();
                    if (this->packet_buf.read_size() > 0) {
                        this->logger->warn("Got packet and break request!");
                    }
                } else {
                    this->resp_buf.reset();
                    try {
                        this->process_pkt();
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
                    auto resp = this->resp_buf.read_buf<std::string>();
                    this->logger->trace("sending response packet:\n\t{}", resp);
                    co_await this->packet_io.send_packet(this->resp_buf);
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
    
    void connection::process_pkt()
    {
        u8 ident = this->packet_buf.get_byte();
        switch (ident) {
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
        std::string append("Fsystem,1337/e\x00 ", 0x20);
        this->append_str(append);
        std::vector<u8> asdf;
        }
        break;

        case read_mem:
        this->append_hex("echo 'hacked';");
        break;

        case get:
        this->handle_q();
        break;

        // case get:
        // this->logger->trace("Queried get");
        // this->append_str("PacketSize=1024");
        // break;

        default:
        std::string res = this->packet_buf.get_str();
        throw unknown_request(fmt::format("ident {:c}, rest: {}", ident, res));
        }
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
} // namespace tasarch::gdb