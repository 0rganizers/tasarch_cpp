#include <mutex>
#include <stdexcept>
#include "gdb/common.h"
#include "packet_io.h"  
#include <asio/awaitable.hpp>
#include <asio/bind_cancellation_slot.hpp>
#include <asio/buffer.hpp>
#include <asio/use_awaitable.hpp>
#include "buffer.h"

namespace tasarch::gdb {
    auto PacketIO::has_buffered_data() -> bool
    {
        return this->read_buf.read_size() > 0;
    }

    auto PacketIO::has_remote_data() -> bool
    {
        return this->socket.available() > 0;
    }

    auto PacketIO::has_data() -> bool
    {
        return this->has_buffered_data() || this->has_remote_data();
    }

    auto PacketIO::recv_data() -> asio::awaitable<void>
    {
        if (this->has_buffered_data()) {
            this->logger->warn("receiving new data, even though we still have {} buffered data available! This is likely a bug!", this->read_buf.read_size());
        }

        this->read_buf.reset();

        this->logger->trace("Receiving up to {} bytes from socket", this->read_buf.write_size());
        size_t num = 0;
            num = co_await awaitable_with_timeout(this->socket.async_receive(this->read_buf.write_buf<asio::mutable_buffer>(), asio::use_awaitable), this->timeout);
        this->logger->trace("Received {} bytes from socket", num);
        if (num < 1) {
            this->logger->warn("Received {} from socket!", num);
        } else {
            this->read_buf.put_count(num);
        }
    }

    auto PacketIO::get_byte() -> asio::awaitable<u8>
    {
        if (!this->has_buffered_data()) {
            /**
             * @todo Should we return nullable here instead, and do that if we dont have remote data?
             */
            co_await this->recv_data();
        }

        if (!this->has_buffered_data()) {
            this->logger->error("Failed to receive data from remote somehow!");
            throw std::runtime_error("failed to receive data from remote somehow");
        }

        co_return this->read_buf.get_byte();
    }

    auto PacketIO::send_packet(buffer &send_buf) -> asio::awaitable<bool>
    {
        /**
        * @todo make this work better. Not sure whether this is fully to spec! (the interrupt detection part)
        */
        bool did_interrupt = false;
        this->logger->trace("sending data sized 0x{:x}", send_buf.read_size());

        /**
            * @todo make it so that we dont have to create send buffer here!
            */

        std::lock_guard lk(this->mutex);

        this->logger->trace("Got lock, writing send buffer to write buffer storage...");

        this->write_buf.reset();
        if (max_packet_expansion(send_buf.read_size()) > this->write_buf.write_size()) {
            this->logger->error("To be sent packet is too large for our write buffer: 1 + 2*{} + 4 = {} > {}", send_buf.read_size(), max_packet_expansion(send_buf.read_size()), this->write_buf.write_size());
            throw std::runtime_error(fmt::format("To be sent packet is too large for our write buffer: 1 + 2*{} + 4 = {} > {}", send_buf.read_size(), max_packet_expansion(send_buf.read_size()), this->write_buf.write_size()));
        }

        size_t len = send_buf.read_size();
        int checksum = 0;
        
        this->write_buf.put_byte(packet_begin);

        // TODO: lets hope compiler optimizes this a bit lul
        for (size_t i = 0; i < len; i++) {
            u8 c = send_buf.get_byte();
            if (must_escape_response(c)) {
                this->write_buf.put_byte(escape);
                checksum += static_cast<u8>(escape);
                c = code_escape_char(c);
            }
            checksum += static_cast<u8>(c);
            // for long packets we could otherwise overflow!
            checksum %= 256;
            this->write_buf.put_byte(c);
        }
        this->write_buf.put_byte(packet_end);
        this->write_buf.put_byte(encode_hex(checksum >> 4));
        this->write_buf.put_byte(encode_hex(checksum >> 0));

        while (true) {

            this->logger->trace("Done writing to write_buf_storage, sending for real...");

            size_t num = co_await awaitable_with_timeout(this->socket.async_send(this->write_buf.read_buf<asio::mutable_buffer>(), asio::use_awaitable), this->timeout);
            if (num != this->write_buf.read_size()) {
                this->logger->error("Could not send everything, wanted to send {}, only sent {}", this->write_buf.read_size(), num);
                // TODO: throw exception here?
            }

            this->logger->trace("Sent data, checking for ack now: {}", this->no_ack);

            if (no_ack) {
                co_return false;
            }

            bool retransmit = false;
            do {
                u8 c = co_await this->get_byte();
                switch (c) {
                case break_character:
                    did_interrupt = true;
                    break;
                case ack:
                    this->logger->trace("got ack!");
                    co_return did_interrupt;
                case ack_err:
                    this->logger->warn("Received ack error, retransmitting...");
                    retransmit = true;
                    break;
                default:
                break;
                }
            } while (!retransmit);
        }
    }

    auto PacketIO::receive_packet(buffer &recv_buf) -> asio::awaitable<bool>
    {
        while (true) {
            if (!this->has_buffered_data()) {
                logger->trace("No buffered data, waiting to receive!");
                co_await this->recv_data();
            }

            std::lock_guard lk(this->mutex);

            if (!this->has_buffered_data() && !this->has_remote_data()) {
                continue;
            }

            enum class State {
                initial,
                packet_data,
                check_hi,
                check_lo,
                escaped
            };

            State state = State::initial;
            u8 checksum = 0;
            int csum_high = -1;
            int csum_low = -1;
            recv_buf.reset();

            while (true) {
                // this seems kinda slow, lets hope it's not actually lul
                u8 c = co_await this->get_byte();
                switch (state) {
                case State::initial:
                {
                    if (c == packet_begin) {
                        state = State::packet_data;
                    } else if (c == break_character) {
                        recv_buf.reset();
                        co_return true;
                    } else {
                        logger->warn("Received char '{:c}' in initial state", c);
                    }
                }
                break;

                case State::packet_data:
                {
                    if (c == packet_end) {
                        state = State::check_hi;
                    } else {
                        checksum += c;
                        if (c == escape) {
                            state = State::escaped;
                        } else {
                            recv_buf.put_byte(c);
                        }
                    }
                }
                break;

                case State::escaped:
                {
                    u8 dec = code_escape_char(c);
                    if (!must_escape_request(dec)) {
                        logger->warn("Got supposedly escaped character 0x{:02x}, decoded to {:c} which is not necessary to escape!", c, dec);
                    }
                    checksum += c;
                    recv_buf.put_byte(dec);
                    state = State::packet_data;
                }
                break;

                case State::check_hi:
                {
                    csum_high = decode_hex(c);
                    state = State::check_lo;
                }
                break;

                case State::check_lo:
                {
                    csum_low = decode_hex(c);
                    /**
                    * @todo Looking at gdbserver source, we should check for potential interrupt after current pkt buffer. Sometimes, interrupt is apparently sent / available with a previous packet.
                    * We dont need to do this, since our gdbstub runs on a separate thread and should be looking at the new command immediately!
                    */

                    if (no_ack) {
                        // TODO: count - 1 here?
                        co_return false;
                    } else {
                        const u8 expectsum = (static_cast<u8>(csum_high) << 4) | (static_cast<u8>(csum_low) << 0);

                        if (csum_high < 0 || csum_low < 0 || checksum != expectsum) {
                            logger->warn("Checksum mismatch 0x{:02x} (expected) vs 0x{:02x} (actual), (hi, lo: {}, {})", expectsum, checksum, csum_high, csum_low);
                            state = State::initial;
                            checksum = 0;
                            csum_high = -1;
                            csum_low = -1;
                            recv_buf.reset();
                            co_await this->socket.async_send(this->ack_err_buf, asio::use_awaitable);
                        } else {
                            logger->trace("Checksum matched successfully, transmitting ack");
                            co_await this->socket.async_send(this->ack_buf, asio::use_awaitable);
                            co_return false;
                        }
                    }
                }
                break;
                }
            }
        }

        co_return false;
    }
} // namespace tasarch::gdb