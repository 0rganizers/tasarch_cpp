#ifndef __SERVER_H
#define __SERVER_H

#include <mutex>
#include <utility>
#include <vector>
#include "log/logging.h"
#include "connection.h"
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
		explicit server(std::shared_ptr<debugger> debugger) : log::WithLogger("gdb.server"), io_context(1), debugger(std::move(debugger)) {}
		asio::ip::port_type port = 5555;

		// just for testing for now.
		bool had_conn = false;

		void start();
		void run();
		void stop();

		auto accept_connection(asio::ip::tcp::acceptor acceptor) -> asio::awaitable<void>;

	private:
		asio::io_context io_context;
		std::vector<std::shared_ptr<connection>> connections;
		std::shared_ptr<std::thread> io_thread = nullptr;
		std::mutex run_mutex;
		bool should_stop = false;
		std::shared_ptr<debugger> debugger;
	};
} // namespace tasarch::gdb



#endif /* __SERVER_H */
