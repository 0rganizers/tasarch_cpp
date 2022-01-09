#ifndef __BG_EXECUTOR_H
#define __BG_EXECUTOR_H

#include <vector>
#include <asio/awaitable.hpp>
#include <asio/execution_context.hpp>
#include <asio/io_context.hpp>
#include "log/logging.h"

namespace tasarch::gdb {
	class bg_executor : log::WithLogger
	{
	public:
		explicit bg_executor(int num_threads = 1) : log::WithLogger("bgexec"), io_context(num_threads), num_threads(num_threads) {}
		asio::io_context io_context;

		void start();
		void run();
		auto idle() -> asio::awaitable<void>;
		void stop();

		static auto instance() -> bg_executor &;

	private:
		int num_threads = 1;
		std::vector<std::shared_ptr<std::thread>> io_threads;
		bool should_stop = false;
		bool running = false;
		bool created_threads = false;
	};
} // namespace tasarch::gdb

#endif /* __BG_EXECUTOR_H */
