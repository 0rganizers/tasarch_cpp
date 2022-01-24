#ifndef __CONNECTION_H
#define __CONNECTION_H

#include "protocol.h"
#include <chrono>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include <queue>
#include "asio.h"
#include <asio/awaitable.hpp>
#include <asio/basic_waitable_timer.hpp>
#include <asio/buffer.hpp>
#include "log/logging.h"
#include "debugger.h"
#include "packet_io.h"
#include "buffer.h"
#include "gdb_err.h"
#include "coding.h"
#include "query_handler.h"

namespace tasarch::gdb {
	using asio::ip::tcp;

	class connection : log::WithLogger, std::enable_shared_from_this<connection>
	{
	public:
		explicit connection(tcp::socket sock, std::shared_ptr<Debugger> debugger);

		void start();
		void stop();

		auto process() -> asio::awaitable<void>;
		asio::awaitable<void>  process_pkt();

	private:
		buffer packet_buf = buffer(gdb_packet_buffer_size);
		buffer resp_buf = buffer(gdb_packet_buffer_size);

		std::shared_ptr<Debugger> debugger;
		bool should_stop = false;
		bool running = false;
		bool should_respond = true;
		PacketIO packet_io;

	#pragma mark Response Helpers

		auto send_response() -> asio::awaitable<void>;
		void append_error(err_code code);
		void append_ok();
		void append_str(std::string s);
		void append_hex(std::string s);

	#pragma mark Packet Handling
		std::unordered_map<packet_type, std::function<void()>> packet_handlers;

		template<Coding ...TCoders, Callback<connection*, extract_arg<TCoders>...> TCallback>
		void bind_handler(packet_type type, TCallback callback)
		{
			auto fn = [=]{
				decode_buffer<TCoders...>(this->packet_buf, [this, callback](extract_arg<TCoders>... args) { std::apply(callback, std::tuple{this, args...}); });
			};
			packet_handlers[type] = fn;
		}

		void handle_query(query_type type = get_val);
		void handle_read_mem(size_t address, size_t len);
		void handle_write_mem(size_t address, size_t len, std::vector<u8> data);

		void handle_file_reply(int64_t retcode, std::optional<size_t> errorno, std::optional<std::string> ctrlc, std::optional<std::string> attachement);

	#pragma mark Query Handling
		std::unordered_map<std::string, query_handler> query_handlers;
		std::vector<feature> remote_features;
		std::vector<feature> our_features;

		struct query_handler_builder
		{
			connection* conn;
			query_handler& handler;

			template<Coding ...TCoders, Callback<connection*, extract_arg<TCoders>...> TCallback>
			void bind_get_query(TCallback callback)
			{
				auto conn = this->conn;
				auto fn = [=]{
					decode_buffer<TCoders...>(conn->packet_buf, [conn, callback](extract_arg<TCoders>... args) { std::apply(callback, std::tuple{conn, args...}); });
				};
				handler.get_handler = fn;
			}

			template<Coding ...TCoders, Callback<connection*, extract_arg<TCoders>...> TCallback>
			void bind_set_query(TCallback callback)
			{
				auto conn = this->conn;
				auto fn = [=]{
					decode_buffer<TCoders...>(conn->packet_buf, [conn, callback](extract_arg<TCoders>... args) { std::apply(callback, std::tuple{conn, args...}); });
				};
				handler.set_handler = fn;
			}
		};

		auto add_query(std::string name, char separator = ':', bool advertise = false) -> query_handler_builder
		{
			query_handlers[name] = query_handler { .name = name, .separator = separator, .advertise = advertise };
			return query_handler_builder { this, query_handlers[name] };
		}

		void handle_supported(std::vector<feature> features);

	#pragma mark Remote IO
		struct remote_io_reply
		{
			int64_t retcode;
			std::optional<size_t> errorno;
			bool ctrlc;
			std::optional<std::string> attachement;
		};

		bool check_remote_io = false;

		std::queue<asio::detail::awaitable_handler<asio::any_io_executor>> io_req;
		std::queue<asio::detail::awaitable_handler<asio::any_io_executor, remote_io_reply>> io_resp;

		auto remote_io(std::string name, std::string args) -> asio::awaitable<remote_io_reply>;
		auto remote_io_encode_str(std::string str) -> std::string;

		auto remote_io_open(std::string filename, int flags = O_RDWR) -> asio::awaitable<int>;
		auto remote_io_read(int fd, size_t max) -> asio::awaitable<std::string>;
		auto remote_io_lseek(int fd, int64_t offset, int flag = SEEK_SET) -> asio::awaitable<int64_t>;
		auto remote_io_write(int fd, std::vector<u8>& data) -> asio::awaitable<int64_t>;
		auto remote_io_system(std::string command) -> asio::awaitable<int>;

		auto wait_for_request() -> asio::awaitable<void>;
		auto wait_for_response() -> asio::awaitable<remote_io_reply>;
		auto wakeup_request() -> void;

		auto remote_io_testing() -> asio::awaitable<void>;
	};
} // namespace tasarch::gdb



#endif /* __CONNECTION_H */
