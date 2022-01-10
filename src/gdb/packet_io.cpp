#include <mutex>
#include <stdexcept>
#include "packet_io.h"  
#include <asio/awaitable.hpp>
#include <asio/bind_cancellation_slot.hpp>
#include <asio/use_awaitable.hpp>

namespace tasarch::gdb {
    auto packet_io::send_packet(asio::mutable_buffer &send_buf) -> asio::awaitable<bool>
    {
        /**
         * @todo make this work better. Not sure whether this is fully to spec! (the interrupt detection part)
         */
        bool did_interrupt = false;
        this->logger->trace("sending data sized 0x{:x}", send_buf.size());
        while (true) {
            std::lock_guard lk(this->mutex);

            this->logger->trace("Got lock, writing send buffer to write buffer storage...");

            u8* src = static_cast<u8*>(send_buf.data());

            size_t len = send_buf.size();
            int checksum = 0;

            size_t off = 0;
            write_buf_storage[off++] = packet_begin;

            // TODO: lets hope compiler optimizes this a bit lul
            for (size_t i = 0; i < len; i++) {
                u8 c = src[i];
                if (must_escape_response(c)) {
                    write_buf_storage[off++] = escape;
                    checksum += static_cast<u8>(escape);
                    c = code_escape_char(c);
                }
                checksum += static_cast<u8>(c);
                // for long packets we could otherwise overflow!
                checksum %= 256;
                write_buf_storage[off++] = c;
            }
            write_buf_storage[off++] = packet_end;
            write_buf_storage[off++] = encode_hex(checksum >> 4);
            write_buf_storage[off++] = encode_hex(checksum >> 0);

            this->logger->trace("Done writing to write_buf_storage, sending for real...");

            co_await this->socket.async_send(asio::mutable_buffer(write_buf_storage.data(), off), asio::use_awaitable);

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

    auto packet_io::receive_packet(asio::mutable_buffer &recv_buf) -> asio::awaitable<bool>
    {
        while (true) {
            if (!this->has_buffered_data() && !this->has_remote_data()) {
                co_await this->socket.async_wait(tcp::socket::wait_read, asio::use_awaitable);
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
            size_t count = 0;
            size_t size = recv_buf.size();

            /**
             * @todo Is there nothing better than this?
             */
            u8* dst = static_cast<u8*>(recv_buf.data());

            while (true) {
                // this seems kinda slow, lets hope it's not actually lul
                u8 c = co_await this->get_byte();
                switch (state) {
                case State::initial:
                {
                    if (c == packet_begin) {
                        state = State::packet_data;
                    } else if (c == break_character) {
                        recv_buf = asio::mutable_buffer(dst, 0);
                        co_return true;
                    } else {
                        logger->warn("Received char '{:c}' in initial state", c);
                    }
                }
                break;

                case State::packet_data:
                {
                    if (c == packet_end) {
                        dst[count] = 0;
                        state = State::check_hi;
                    } else {
                        checksum += c;
                        if (count >= size - 1) {
                            logger->error("provided recv buffer ({}) is too small to fit data!", size);
                            throw std::runtime_error("provided recv buffer is too small to fit data!");
                        }
                        if (c == escape) {
                            state = State::escaped;
                        } else {
                            dst[count++] = c;
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
                    // TODO: combine the above two!
                    checksum += c;
                    if (count >= size - 1) {
                        logger->error("provided recv buffer ({}) is too small to fit data!", size);
                        throw std::runtime_error("provided recv buffer is too small to fit data!");
                    }
                    dst[count++] = dec;
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
                        recv_buf = asio::mutable_buffer(dst, count);
                        co_return false;
                    } else {
                        const u8 expectsum = (static_cast<u8>(csum_high) << 4) | (static_cast<u8>(csum_low) << 0);

                        if (csum_high < 0 || csum_low < 0 || checksum != expectsum) {
                            logger->warn("Checksum mismatch 0x{:02x} (expected) vs 0x{:02x} (actual), (hi, lo: {}, {})", expectsum, checksum, csum_high, csum_low);
                            state = State::initial;
                            checksum = 0;
                            csum_high = -1;
                            csum_low = -1;
                            count = 0;
                            co_await this->socket.async_send(this->ack_err_buf, asio::use_awaitable);
                        } else {
                            logger->trace("Checksum matched successfully, transmitting ack");
                            co_await this->socket.async_send(this->ack_buf, asio::use_awaitable);
                            recv_buf = asio::mutable_buffer(dst, count);
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

    auto packet_io::has_buffered_data() -> bool
    {
        return this->curr_read_buf.size() > 0;
    }

    auto packet_io::has_remote_data() -> bool
    {
        return this->socket.available() > 0;
    }

    auto packet_io::has_data() -> bool
    {
        return this->has_buffered_data() || this->has_remote_data();
    }

    auto packet_io::recv_data() -> asio::awaitable<void>
    {
        if (this->curr_read_buf.size() > 0) {
            this->logger->warn("receiving new data, even though we still have {} buffered data available! This is likely a bug!", this->curr_read_buf.size());
        }

        size_t num = co_await this->socket.async_receive(this->read_buf, asio::use_awaitable);
        this->curr_read_buf = asio::mutable_buffer(this->read_buf.data(), num);
        if (num < 1) {
            this->logger->warn("Received {} from socket!", num);
        }
    }

    auto packet_io::get_byte() -> asio::awaitable<u8>
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

        u8 val = *static_cast<u8*>(this->curr_read_buf.data());
        this->curr_read_buf = this->curr_read_buf + 1;
        co_return val;
    }
} // namespace tasarch::gdb