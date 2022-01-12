#ifndef __CONNECTION_H
#define __CONNECTION_H

#include <memory>
#include <utility>
#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include "log/logging.h"
#include "debugger.h"
#include "packet_io.h"
#include "buffer.h"
#include "gdb_err.h"

namespace tasarch::gdb {
	using asio::ip::tcp;

	enum packet_type : u8
	{
		enable_extended = '!',
		query_stop_reason = '?',
		cont = 'c',
		cont_sig = 'C',
		detach = 'D',
		file_io = 'F',
		read_gpr = 'g',
		write_gpr = 'G',
		set_thd = 'H',
		step_cycle = 'i',
		step_cycle_sig = 'I',
		kill = 'k',
		read_mem = 'm',
		write_mem = 'M',
		read_reg = 'p',
		write_reg = 'P',
		get = 'q',
		set = 'Q',
		reset = 'r',
		restart = 'R',
		step = 's',
		step_sig = 'S',
		search = 't',
		query_thd = 'T',
		vcont = 'v',
		write_mem_bin = 'X',
		add_break = 'z',
		del_break = 'Z'
	};

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
		buffer<gdb_packet_buffer_size> packet_buf;
		buffer<gdb_packet_buffer_size> resp_buf;

		std::shared_ptr<Debugger> debugger;
		bool should_stop = false;
		bool running = false;
		PacketIO packet_io;

		void append_error(err_code code);
		void append_ok();
		void append_str(std::string s);
		void append_hex(std::string s);
	};
} // namespace tasarch::gdb



#endif /* __CONNECTION_H */
