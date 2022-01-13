#ifndef __CONNECTION_H
#define __CONNECTION_H

#include <memory>
#include <utility>
#include "asio.h"
#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include "log/logging.h"
#include "debugger.h"
#include "packet_io.h"
#include "buffer.h"
#include "gdb_err.h"
#include "protocol.h"
#include "coding.h"

namespace tasarch::gdb {
	using asio::ip::tcp;

	class connection : log::WithLogger, std::enable_shared_from_this<connection>
	{
	public:
		explicit connection(tcp::socket sock, std::shared_ptr<Debugger> debugger) : log::WithLogger("gdb.conn"), debugger(std::move(debugger)), packet_io(sock)
		{
		}

		void start();
		void stop();

		auto process() -> asio::awaitable<void>;
		void process_pkt();

	private:
		buffer packet_buf = buffer(gdb_packet_buffer_size);
		buffer resp_buf = buffer(gdb_packet_buffer_size);

		std::shared_ptr<Debugger> debugger;
		bool should_stop = false;
		bool running = false;
		PacketIO packet_io;

	#pragma mark Response Helpers

		void append_error(err_code code);
		void append_ok();
		void append_str(std::string s);
		void append_hex(std::string s);

	#pragma mark Packet Handling
		void handle_q(query_type type = get_val);
	};
} // namespace tasarch::gdb



#endif /* __CONNECTION_H */
