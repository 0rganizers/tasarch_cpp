#ifndef __PACKET_IO_H
#define __PACKET_IO_H

#include <array>
#include <chrono>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include "util/literals.h"
#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include <fmt/core.h>
#include "log/logging.h"
#include "util/defines.h"
#include "buffer.h"
#include "common.h"

using namespace std::chrono_literals;

namespace tasarch::gdb {
	/**
	 * @brief Size of the buffer for actual command packets.
	 *
	 * Size copied from atmosphere code, maybe we should look into what is best for our purposes?
	 * 
	 */
	static constexpr size_t gdb_packet_buffer_size = 32_KB;

	/**
	 * @brief Size of the buffer for receiving on the socket (so raw data).
	 *
	 * Also copied from atmosphere code.
	 * 
	 */
	static constexpr size_t gdb_transport_buffer_size = 4_KB;

	/**
	 * @brief Returns the maximum number of bytes a packet of size packet_size can blow up due to escaping and checksum, etc.
	 * 
	 * It is calculated as `1 + 2*packet_size + 4` (packet_begin + escaping + checksum)
	 * @param packet_size 
	 * @return size_t 
	 */
	constexpr auto max_packet_expansion(size_t packet_size) -> size_t
	{
		return 1 + 2*packet_size + 4;
	}

	/**
	 * To make things easier to type.
	 */
	using asio::ip::tcp;

	/**
	 * @brief Byte received when gdb client requests an interrupt.
	 * 
	 * The gdb client sends this on a ctrl-c on its end.
	 * Through this, it can tell the server to halt execution of its target, allowing things like adding breakpoints again (e.g. you messed up when adding them or dont trust gdb that they werent hit ;)).
	 */
	constexpr u8 break_character = '\x03'; /* ctrl-c */

	/**
	 * @brief Simple hex decoding of a nibble.
	 *
	 * @todo Borrowed from atmosphere.
	 * 
	 * @param c The character.
	 * @return int Either the decoded nibble (`[0, 15]`) or `-1` on failure.
	 */
	constexpr auto decode_hex(u8 c) -> int {
		if ('a' <= c && c <= 'f') {
			return 10 + (c - 'a');
		} else if ('A' <= c && c <= 'F') {
			return 10 + (c - 'A');
		} else if ('0' <= c && c <= '9') {
			return 0 + (c - '0');
		} else {
			return -1;
		}
	}

	/**
	 * @brief Simple hex encoding of a nibble.
	 *
	 * @todo Borrowed from atmosphere.
	 * 
	 * @param v The value.
	 * @note Even values beyond `[0, 15]` will be encoded, `v` is just `& 0xf`'d.
	 * @return char 
	 */
	constexpr auto encode_hex(u8 v) -> char {
		return "0123456789abcdef"[v & 0xF];
	}

	/**
	 * @brief Implements the low-level transport protocol as specified by GDB.
	 * For details see https://sourceware.org/gdb/onlinedocs/gdb/Overview.html#Overview
	 * 
	 * Basically, you give it a socket and you can then send and receive packets, going through the encoding mentioned in the link above.
	 *
	 * @note Whenever we speak of request below, this means a communcation from gdb -> gdbserver, a response goes from gdbserver -> gdb (makes sense, right?).
	 *
	 * @todo For extra swag, make it more generic, i.e. accept stuff like a serial port as well :)
	 */
	class PacketIO : log::WithLogger
	{
	public:

		/**
		 * @brief Various control characters relevant to the protocol.
		 */
		enum control_char : u8
		{
			/**
			 * @brief Marks the beginning of a packet.
			 */
			packet_begin = '$',

			/**
			 * @brief Marks the end of packet data and beginning of checksum.
			 */
			packet_end = '#',

			/**
			 * @brief Ack last packet as good
			 */
			ack = '+',

			/**
			 * @brief Demand resend of last packet (should mostly be due to checksum mismatch)
			 */
			ack_err = '-',

			/**
			 * @brief Used to escape other control characters.
			 */
			escape = '}',

			/**
			 * @brief Marks RLE bytes.
			 * @todo Do we want to implement RLE for sending stuff?
			 */
			rle = '*'
		};

		/**
		 * @brief Whether the given character must be escaped in a request (gdb -> gdbserver)
		 * 
		 * @param c 
		 * @return true 
		 * @return false 
		 */
		static constexpr auto must_escape_request(u8 c) -> bool
		{
			return c == packet_begin || c == packet_end || c == escape;
		}

		/**
		 * @brief Whether the given character must be escaped in a response (gdbserver -> gdb)
		 * 
		 * @param c 
		 * @return true 
		 * @return false 
		 */
		static constexpr auto must_escape_response(u8 c) -> bool
		{
			return must_escape_request(c) || c == rle;
		}

		/**
		 * @brief Escape / unescape the given character.
		 *
		 * We can use the same function for both, since its just a xor :)
		 * 
		 * @param c 
		 * @return u8 
		 */
		static constexpr auto code_escape_char(u8 c) -> u8
		{
			return c ^ 0x20;
		}

		explicit PacketIO(tcp::socket& socket) : log::WithLogger("gdb.io"),
			socket(std::move(socket))
		{
			// this->read_buf_storage.fill(0);
			// this->read_buf = asio::mutable_buffer(read_buf_storage.data(), gdb_transport_buffer_size);
			// this->write_buf_storage.fill(0);
			// this->write_buf = asio::mutable_buffer(write_buf_storage.data(), write_buf_storage.size());
			// // current needs to be empty ya dingus!
			// this->curr_read_buf = asio::mutable_buffer(this->read_buf.data(), 0);
			this->ack_buf = asio::const_buffer(&ack_storage, 1);
			this->ack_err_buf = asio::const_buffer(&ack_err_storage, 1);
		}

		/**
		 * @brief one byte buffer containing the `ack` character.
		 * 
		 * Used for easy sending of acks.
		 * 
		 */
		asio::const_buffer ack_buf;

		/**
		 * @brief one byte buffer containing the `nack` character.
		 * 
		 * Used for easy sending of nacks.
		 * 
		 */
		asio::const_buffer ack_err_buf;

		/**
		 * @brief The timeout after which `receive_packet()` or `send_packet()` should throw a `timed_out` exception.
		 * While no implementation (I know of) actually does this, it makes here imo.
		 * If one end dies, we dont want to hang forever.
		 * Furthermore, this should always be local communication anyways, so 5 seconds is plenty of time.
		 */
		std::chrono::milliseconds timeout = 5000ms;

		tcp::socket socket;

		/**
		 * @brief Disable sending of acks. Also disables checksum checking!
		 * 
		 * See https://sourceware.org/gdb/onlinedocs/gdb/Packet-Acknowledgment.html#Packet-Acknowledgment for more.
		 */
		void set_no_ack() { no_ack = true; }

	private:
		bool no_ack = false;

		/**
		 * @brief Buffer contents received from transport layer here.
		 * @todo Figure out a better way to initialize this?
		 */
		// asio::mutable_buffer read_buf;
		// std::array<u8, gdb_transport_buffer_size> read_buf_storage{};
		// asio::mutable_buffer curr_read_buf;

		buffer<gdb_transport_buffer_size> read_buf;
		buffer<max_packet_expansion(gdb_packet_buffer_size)> write_buf;

		// asio::mutable_buffer write_buf;
		// std::array<u8, 1 + 2*gdb_packet_buffer_size + 4> write_buf_storage{};

		std::mutex mutex;
		const char ack_storage = ack;
		const char ack_err_storage = ack_err;

		auto has_buffered_data() -> bool;
		auto has_remote_data() -> bool;
		auto has_data() -> bool;
		auto recv_data() -> asio::awaitable<void>;
		auto get_byte() -> asio::awaitable<u8>;

	public:
		/**
		 * @brief Receive the latest packet into `recv_buf` and check whether an interrupt was encountered (see `break_character`).
		 * @throws timed_out When the timeout given by `timeout` is reached.
		 * @throws buffer_too_small When the given input buffer is not large enough for the full packet.
		 * @param recv_buf Buffer that will be holding the received packet data. Make it at least `gdb_packet_buffer_size` big.
		 * @warning The buffer needs to have preallocated storage, that also needs to be large enough!
		 * Furthermore, be sure to keep a backup, since the `size()` of the buffer will be changed to the packet size!
		 * @return asio::awaitable<bool> Whether a `break_character` was encountered or not.
		 */
	    template<size_t Size>
		auto send_packet(buffer<Size> &send_buf) -> asio::awaitable<bool>
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

		/**
		 * @brief Send the given data and check whether an interrupt was encountered (see `break_character`).
		 * @throws timed_out When the timeout given by `timeout` is reached.
		 * @param recv_buf Buffer that holds the packet to send.
		 * @return asio::awaitable<bool> Whether a `break_character` was encountered or not.
		 */
		template<size_t Size>
		auto receive_packet(buffer<Size> &recv_buf) -> asio::awaitable<bool>
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
	};
} // namespace tasarch::gdb

#endif /* __PACKET_IO_H */
