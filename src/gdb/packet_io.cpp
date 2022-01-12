#include <mutex>
#include <stdexcept>
#include "gdb/common.h"
#include "packet_io.h"  
#include <asio/awaitable.hpp>
#include <asio/bind_cancellation_slot.hpp>
#include <asio/buffer.hpp>
#include <asio/use_awaitable.hpp>
#include "buffer.h"

namespace tasarch::gdb {
    auto PacketIO::has_buffered_data() -> bool
    {
        return this->read_buf.read_size() > 0;
    }

    auto PacketIO::has_remote_data() -> bool
    {
        return this->socket.available() > 0;
    }

    auto PacketIO::has_data() -> bool
    {
        return this->has_buffered_data() || this->has_remote_data();
    }

    auto PacketIO::recv_data() -> asio::awaitable<void>
    {
        if (this->has_buffered_data()) {
            this->logger->warn("receiving new data, even though we still have {} buffered data available! This is likely a bug!", this->read_buf.read_size());
        }

        this->read_buf.reset();

        this->logger->trace("Receiving up to {} bytes from socket", this->read_buf.write_size());
        size_t num = 0;
            num = co_await awaitable_with_timeout(this->socket.async_receive(this->read_buf.write_buf<asio::mutable_buffer>(), asio::use_awaitable), this->timeout);
        this->logger->trace("Received {} bytes from socket", num);
        if (num < 1) {
            this->logger->warn("Received {} from socket!", num);
        } else {
            this->read_buf.put_count(num);
        }
    }

    auto PacketIO::get_byte() -> asio::awaitable<u8>
    {
        if (!this->has_buffered_data()) {
            /**
             * @todo Should we return nullable here instead, and do that if we dont have remote data?
             */
            co_await this->recv_data();
        }

        if (!this->has_buffered_data()) {
            this->logger->error("Failed to receive data from remote somehow!");
            throw std::runtime_error("failed to receive data from remote somehow");
        }

        co_return this->read_buf.get_byte();
    }
} // namespace tasarch::gdb