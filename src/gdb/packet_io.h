#ifndef __PACKET_IO_H
#define __PACKET_IO_H

#include <array>
#include <chrono>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include "util/literals.h"
#include "asio.h"
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

		/**
		 * @brief Receive the latest packet into `recv_buf` and check whether an interrupt was encountered (see `break_character`).
		 * @throws timed_out When the timeout given by `timeout` is reached.
		 * @throws buffer_too_small When the given input buffer is not large enough for the full packet.
		 * @param recv_buf Buffer that will be holding the received packet data. Make it at least `gdb_packet_buffer_size` big.
		 * @warning The buffer needs to have preallocated storage, that also needs to be large enough!
		 * Furthermore, be sure to keep a backup, since the `size()` of the buffer will be changed to the packet size!
		 * @return asio::awaitable<bool> Whether a `break_character` was encountered or not.
		 */
		auto send_packet(buffer &send_buf) -> asio::awaitable<bool>;
		
		/**
		 * @brief Send the given data and check whether an interrupt was encountered (see `break_character`).
		 * @throws timed_out When the timeout given by `timeout` is reached.
		 * @param recv_buf Buffer that holds the packet to send.
		 * @return asio::awaitable<bool> Whether a `break_character` was encountered or not.
		 */
		auto receive_packet(buffer &recv_buf) -> asio::awaitable<bool>;

	private:
		bool no_ack = false;

		/**
		 * @brief Buffer contents received from transport layer here.
		 * @todo Figure out a better way to initialize this?
		 */
		// asio::mutable_buffer read_buf;
		// std::array<u8, gdb_transport_buffer_size> read_buf_storage{};
		// asio::mutable_buffer curr_read_buf;

		buffer read_buf = buffer(gdb_transport_buffer_size);
		buffer write_buf = buffer(max_packet_expansion(gdb_packet_buffer_size));

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
	};
} // namespace tasarch::gdb

#endif /* __PACKET_IO_H */
