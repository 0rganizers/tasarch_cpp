#ifndef __SERVER_H
#define __SERVER_H

#include "connection.h"
#include <mutex>
#include <utility>
#include <vector>
#include "log/logging.h"
#include <asio.hpp>
#include <asio/awaitable.hpp>
#include <asio/execution_context.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/basic_endpoint.hpp>
#include "debugger.h"

namespace tasarch::gdb {
	class server : log::WithLogger
	{
	public:
		explicit server(std::shared_ptr<Debugger> debugger) : log::WithLogger("gdb.server"), debugger(std::move(debugger)) {}
		asio::ip::port_type port = 5555;

		void start();
		void stop();

		auto accept_connection(asio::ip::tcp::acceptor acceptor) -> asio::awaitable<void>;

	private:
		std::vector<std::shared_ptr<connection>> connections;
		std::mutex run_mutex;
		bool running = false;
		std::shared_ptr<Debugger> debugger;
	};
} // namespace tasarch::gdb



#endif /* __SERVER_H */
