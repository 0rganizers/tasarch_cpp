#ifndef __GDB_BUFFER_H
#define __GDB_BUFFER_H

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <asio.hpp>
#include <asio/buffer.hpp>
#include "util/defines.h"
#include <fmt/core.h>
#include <concepts>
#include <string>

namespace tasarch::gdb {
    /**
	 * @brief Exception thrown when a buffer was too small to fit all recvd data.
	 * 
	 */
	class buffer_too_small : std::runtime_error
	{
	public:
		/**
		 * @brief Construct a new exception.
		 * 
		 * @param buf_size The size of the buffer, which was too small.
		 */
		explicit buffer_too_small(size_t buf_size) : std::runtime_error(fmt::format("provided buffer (0x{:x}) too small", buf_size)) {}
	};

    template <typename From, typename To>
    concept convertible_to = std::is_convertible_v<From, To>;

    template<typename TBuffer>
    concept Bufferable = requires(TBuffer buf, u8* ptr, size_t num) {
        {buf.data()} -> convertible_to<void*>;
        {buf.size()} -> convertible_to<size_t>;
        {TBuffer((decltype(buf.data()))ptr, num)} -> std::same_as<TBuffer>;
    };

    template <size_t Size>
    class buffer
    {
public:
#pragma mark Simple Accessors

        auto read_size() -> size_t
        {
            return this->write_off - this->read_off;
        }

        auto read_data() -> const u8*
        {
            return this->storage.data() + this->read_off;
        }

        template<typename TBuffer>
        auto read_buf() -> TBuffer requires Bufferable<TBuffer>
        {
            return TBuffer((decltype(TBuffer().data()))(this->read_data()), this->read_size());
        }

        auto write_size() -> size_t
        {
            return Size - this->write_off;
        }

        auto write_data() -> u8*
        {
            return this->storage.data() + this->write_off;
        }

        template<typename TBuffer>
        auto write_buf() -> TBuffer requires Bufferable<TBuffer>
        {
            return TBuffer((decltype(TBuffer().data()))this->write_data(), this->write_size());
        }

#pragma mark Get Functions

        auto get_byte() -> u8
        {
            if (this->read_size() < 1) {
                throw std::out_of_range("No more bytes left!");
            }
            u8 val = this->storage.at(this->read_off);
            this->read_off++;
            return val;
        }

        template<typename TBuffer>
        auto get_buf(size_t num) -> TBuffer requires Bufferable<TBuffer>
        {
            if (this->read_size() < num) {
                throw std::out_of_range(fmt::format("Not enough bytes in buffer: {} < {}", this->read_size(), num));
            }
            TBuffer ret((decltype(TBuffer().data()))this->read_data(), num);
            this->read_off += num;
            return ret;
        }

        auto get_str(size_t num = 0) -> std::string
        {
            if (num == 0) { num = this->read_size(); }
            auto str = this->get_buf<std::string>(num);
            return str;
        }

        void get_count(size_t num)
        {
            if (this->read_size() < num) {
                throw std::out_of_range(fmt::format("Not enough bytes in buffer: {} < {}", this->read_size(), num));
            }
            this->read_off += num;
        }

#pragma mark Put Functions

        void put_byte(u8 val)
        {
            if (this->write_size() < 1) {
                throw buffer_too_small(Size);
            }
            this->storage.at(this->write_off) = val;
            this->write_off++;
        }

        template<typename TBuffer>
        void append_buf(TBuffer& buf) requires Bufferable<TBuffer>
        {
            if (this->write_size() < buf.size()) {
                throw buffer_too_small(this->write_size());
            }
            std::memcpy(this->write_data(), buf.data(), buf.size());
            this->write_off += buf.size();
        }

        void put_count(size_t num)
        {
            if (this->write_size() < num) {
                // TODO: Kinda too late at this point lmao
                throw buffer_too_small(this->write_size());
            }
            this->write_off += num;
        }

#pragma mark Miscellaneous Functions

        void reset()
        {
            this->write_off = 0;
            this->read_off = 0;
        }

    private:
        size_t write_off = 0;
        size_t read_off = 0;
        std::array<u8, Size> storage;
    };

} // namespace tasarch::gdb


#endif /* __GDB_BUFFER_H */