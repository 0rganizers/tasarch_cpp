#ifndef __CONNECTION_H
#define __CONNECTION_H

#include <memory>
#include <utility>
#include <asio/awaitable.hpp>
#include "log/logging.h"
#include "debugger.h"
#include "packet_io.h"

namespace tasarch::gdb {
	using asio::ip::tcp;

	class connection : log::WithLogger, std::enable_shared_from_this<connection>
	{
	public:
		explicit connection(tcp::socket sock, std::shared_ptr<debugger> debugger) : log::WithLogger("gdb.conn"), socket(std::move(sock)), debugger(std::move(debugger)), packet_io(socket) {}

		tcp::socket socket;

		void start();
		void stop();

		auto process() -> asio::awaitable<void>;

	private:
		std::shared_ptr<debugger> debugger;
		bool should_stop = false;
		bool running = false;
		packet_io packet_io;
	};
} // namespace tasarch::gdb



#endif /* __CONNECTION_H */
