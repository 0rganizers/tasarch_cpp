#ifndef __PACKET_IO_H
#define __PACKET_IO_H

#include <array>
#include <cstddef>
#include "util/literals.h"
#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include "log/logging.h"
#include "util/defines.h"

namespace tasarch::gdb {
	static constexpr size_t gdb_packet_buffer_size = 32_KB;
	static constexpr size_t gdb_transport_buffer_size = 4_KB;

	using asio::ip::tcp;

	constexpr u8 break_character = '\x03'; /* ctrl-c */

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

	constexpr auto encode_hex(u8 v) -> char {
		return "0123456789abcdef"[v & 0xF];
	}

	class packet_io : log::WithLogger
	{
	public:
		enum control_char : u8
		{
			packet_begin = '$',
			packet_end = '#',
			ack = '+',
			ack_err = '-',
			escape = '}',
			rle = '*'
		};

		static constexpr auto must_escape_request(u8 c) -> bool
		{
			return c == packet_begin || c == packet_end || c == escape;
		}

		static constexpr auto must_escape_response(u8 c) -> bool
		{
			return must_escape_request(c) || c == rle;
		}

		static constexpr auto code_escape_char(u8 c) -> u8
		{
			return c ^ 0x20;
		}

		explicit packet_io(tcp::socket& socket) : log::WithLogger("gdb.io"),
			socket(std::move(socket))
		{
			this->read_buf_storage.fill(0);
			this->read_buf = asio::mutable_buffer(read_buf_storage.data(), gdb_transport_buffer_size);
			this->write_buf_storage.fill(0);
			this->write_buf = asio::mutable_buffer(write_buf_storage.data(), write_buf_storage.size());
			// current needs to be empty ya dingus!
			this->curr_read_buf = asio::mutable_buffer(this->read_buf.data(), 0);
			this->ack_buf = asio::const_buffer(&ack_storage, 1);
			this->ack_err_buf = asio::const_buffer(&ack_err_storage, 1);
		}

		asio::const_buffer ack_buf;
		asio::const_buffer ack_err_buf;

		void set_no_ack() { no_ack = true; }

		auto receive_packet(asio::mutable_buffer& recv_buf) -> asio::awaitable<bool>;
		auto send_packet(asio::mutable_buffer& send_buf) -> asio::awaitable<bool>;

	private:
		bool no_ack = false;
		tcp::socket socket;

		/**
		 * @brief Buffer contents received from transport layer here.
		 * @todo Figure out a better way to initialize this?
		 */
		asio::mutable_buffer read_buf;
		std::array<u8, gdb_transport_buffer_size> read_buf_storage{};
		asio::mutable_buffer curr_read_buf;

		asio::mutable_buffer write_buf;
		std::array<u8, 1 + 2*gdb_packet_buffer_size + 4> write_buf_storage{};

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
