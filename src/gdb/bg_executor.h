#ifndef __BG_EXECUTOR_H
#define __BG_EXECUTOR_H

#include <vector>
#include "clangd_fix.h"
#include <asio/awaitable.hpp>
#include <asio/execution_context.hpp>
#include <asio/io_context.hpp>
#include "log/logging.h"

namespace tasarch::gdb {
	/**
	 * @brief Responsible for handling multiple threads, that will then run coroutines (mostly io, mostly for gdbstub).
	 * Normally using asio, you would just run the executor inside the main thread.
	 * However, here the main thread belongs to the GUI, so we create background thread(s) to run the executor.
	 *
	 * This has the added benefit, of no coroutines ever blocking the GUI and we can even completely reset the coroutine state, by restarting the bg_executor.
	 * This could be useful, if e.g. the state of the gdbstub gets very screwed up, since it would be annoying having to restart the emulator due to that.
	 * Especially, if you would have to force kill because the GUI became unresponsive (looking at you, Citra!).
	 *
	 * @note Whenever I refer to "coroutines" here, I do not necessarily mean actual C++20 coroutines, but rather anything that resembles that.
	 * For example, this also includes an async socket read, which I think is not actually a C++20 coroutine?
	 */
	class bg_executor : log::WithLogger
	{
	public:
		/**
		 * @brief Create a new bg executor.
		 * @note You shouldnt really be calling this, instead use `instance()`!
		 * 
		 * @param num_threads The number of threads used for running coroutines. Ideally we want this configurable at some point, but it needs to be set in the constructor :/.
		 * Set to 2 for now, maybe that makes concurrency bugs appear more often?
		 */
		explicit bg_executor(int num_threads = 2) : log::WithLogger("bgexec"), io_context(num_threads), num_threads(num_threads) {}

		/**
		 * @brief The `asio::io_context` used for any coroutines. Only really needed for constructing io objects, I think?
		 * 
		 */
		asio::io_context io_context;

		/**
		 * @brief Start the background threads handling coroutines.
		 * @warning This function is not thread safe, should really only be called from the main thread.
		 */
		void start();

		/**
		 * @brief The internal run function, any thread handling coroutines executes.
		 * @todo Make this private!
		 */
		void run();

		/**
		 * @brief Idle coroutine, launched so we always have work to do.
		 *
		 * `asio::io_context.run()` tries to be smart and will exit if there is no work left to do.
		 * This means, `run()` will also exit. Therefore, we have this simple idle coroutine, that just waits for a million+ years timer to expire :)
		 * 
		 * @return asio::awaitable<void> 
		 */
		auto idle() -> asio::awaitable<void>;

		/**
		 * @brief Stop the background threads handling coroutines. Waits for them to exit.
		 * @warning This function is not thread safe, should really only be called from the main thread.
		 */
		void stop();

		/**
		 * @brief The global shared instance that should be used across the project.
		 *
		 * Necessary, so we dont have to pass this around all the time.
		 * 
		 * @return bg_executor& 
		 */
		static auto instance() -> bg_executor &;

	private:
		/**
		 * @brief Number of `io_threads` to spawn.
		 *
		 * See `bg_executor()` for details.
		 * 
		 */
		int num_threads;

		/**
		 * @brief List of currently active io_threads. Used to stop all of them.
		 * 
		 */
		std::vector<std::shared_ptr<std::thread>> io_threads;

		bool should_stop = false;
		bool running = false;
		bool created_threads = false;
	};
} // namespace tasarch::gdb

#endif /* __BG_EXECUTOR_H */
