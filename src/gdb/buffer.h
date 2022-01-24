#ifndef __GDB_BUFFER_H
#define __GDB_BUFFER_H

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include "asio.h"
#include <asio.hpp>
#include <asio/buffer.hpp>
#include "util/defines.h"
#include <fmt/core.h>
#include <concepts>
#include <string>
#include "util/warnings.h"
#include "util/concepts.h"

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

    /**
     * @concept Bufferable
     * @brief Concept for a type that is like a `std::string` or `std::vector`.
     * In other words, it has a data pointer, a size and can be constructed from data pointer and size.
     * 
     * @tparam TBuffer 
     */
    template<typename TBuffer>
    concept Bufferable = requires(TBuffer buf, u8* ptr, size_t num) {
        {buf.data()} -> convertible_to<void*>;
        {buf.size()} -> convertible_to<size_t>;
        {TBuffer((decltype(buf.data()))ptr, num)} -> std::same_as<TBuffer>;
    };

    /**
     * @brief Buffer with static storage, that allows both reading and writing to it, while keeping track of current read and write head.
     *
     * In essence, it provides many convenience methods on top of an `std::vector`, while also keeping track of the current read and write head.
     *
     * @todo Not yet fully happy with this. I think maybe we can refactor it a bit / make it work better across the whole project?
     * 
     */
    class buffer
    {
public:
        /**
         * @brief Construct a new buffer object with the given size.
         * 
         * @param size 
         */
        explicit buffer(const size_t size) : max_size(size)
        {
            this->storage.resize(size);
        }

        /**
         * @name Simple Accessors 
         * Simple accessors to the underlying storage and sizes.
         * Should probably use more high level methods instead.
         */
        ///@{
#pragma mark Simple Accessors

        /**
         * @brief The current amount of data that can be read.
         * 
         * @return size_t 
         */
        [[nodiscard]] auto read_size() const -> size_t
        {
            return this->write_off - this->read_off;
        }

        /**
         * @brief Pointer directly to the currently still unread data.
         * 
         * @return const u8* 
         */
        auto read_data() -> const u8*
        {
            return this->storage.data() + this->read_off;
        }

        /**
         * @brief Returns a newly created `TBuffer` object, containing all unread data.
         *
         * \warning Depending on `TBuffer`, this might be a copy of the data and not just transparently using the same pointer.
         * For example, `std::string` will create a copy of the underlying data.
         * Note that this may also be desirable, for example if you want to keep the data around for longer.
         * In essence, this function is very dumb, it just calls `TBuffer(read_data(), read_size())`.
         *
         * This function is mostly useful with `asio` functions, since a `asio::mutable_buffer` does no copies.
         * 
         * @tparam TBuffer 
         * @return TBuffer requires Bufferable<TBuffer> 
         */
        template<typename TBuffer>
        auto read_buf() -> TBuffer requires Bufferable<TBuffer>
        {
            return TBuffer((decltype(TBuffer().data()))(this->read_data()), this->read_size());
        }

        /**
         * @brief The current free space we can write to.
         * 
         * @return size_t 
         */
        [[nodiscard]] auto write_size() const -> size_t
        {
            return max_size - this->write_off;
        }

        /**
         * @brief Pointer directly to the next byte we should write to.
         * 
         * @return u8* 
         */
        auto write_data() -> u8*
        {
            return this->storage.data() + this->write_off;
        }

        /**
         * @brief Returns a newly created `TBuffer` object, containing all not yet written data.
         *
         * @warning See the warning mentioned for `read_buf()` it also applies here.
         * 
         * @tparam TBuffer 
         * @return TBuffer requires Bufferable<TBuffer> 
         */
        template<typename TBuffer>
        auto write_buf() -> TBuffer requires Bufferable<TBuffer>
        {
            return TBuffer((decltype(TBuffer().data()))this->write_data(), this->write_size());
        }

        ///@}


        /**
         * @name Get Functions
         * Higher level functions for reading / getting data out of the buffer.
         */
        ///@{
#pragma mark Get Functions

        /**
         * @brief Get one byte from the buffer and increment the current read head.
         * @throws std::out_of_range when there are no bytes left to be read.
         * @return u8 
         */
        auto get_byte() -> u8
        {
            if (this->read_size() < 1) {
                throw std::out_of_range("No more bytes left!");
            }
            u8 val = this->storage.at(this->read_off);
            this->read_off++;
            return val;
        }

        /**
         * @brief Convenience function to ensure that at least `num` bytes are available to read.
         * @throws std::out_of_range if not enough bytes are available.
         * @param num 
         */
        void read_require(size_t num) const
        {
            if (this->read_size() < num) {
                throw std::out_of_range(fmt::format("Not enough bytes in buffer: {} < {}", this->read_size(), num));
            }
        }

        /**
         * @brief Convenience function to advance the read head by `num` bytes.
         * Used in tandem with the low level functions (e.g. `read_buf()`) that do not automatically increase the read head.
         * @throws std::out_of_range if not enough bytes are available.
         * @param num 
         */
        void get_count(size_t num)
        {
            this->read_require(num);
            this->read_off += num;
        }

        /**
         * @brief Advance the read head by `num` bytes and return a new `TBuffer` object containing the bytes between old and new read head.
         * @warning Again, the same limitation as for `read_buf()` apply.
         @throws std::out_of_range if not enough bytes are available.
         * @tparam TBuffer 
         * @param num 
         * @return TBuffer requires Bufferable<TBuffer> 
         */
        template<typename TBuffer>
        auto get_buf(size_t num) -> TBuffer requires Bufferable<TBuffer>
        {
            this->read_require(num);
            TBuffer ret((decltype(TBuffer().data()))this->read_data(), num);
            this->read_off += num;
            return ret;
        }

        /**
         * @brief Convenience function for consuming and returning `num` bytes of the current read buffer as an `std::string`.
         * If `num = 0`, then all of the current read buffer will be returned.
         *
         * @note This is a convenience wrapper around `get_buf()`, so the bytes will be consumed and not available for reading afterwards.
         * 
         @throws std::out_of_range if not enough bytes are available.
         * @param num 
         * @return std::string 
         */
        auto get_str(size_t num = 0) -> std::string
        {
            if (num == 0) { num = this->read_size(); }
            auto str = this->get_buf<std::string>(num);
            return str;
        }

        /**
         * @brief Copy `sizeof(T)` bytes into `*elem` and consume them.
         * @throws std::out_of_range if not enough bytes are available.
         * @tparam T 
         * @param elem 
         * @return auto 
         */
        template<typename T>
        auto read_into(T* elem)
        {
            size_t num = sizeof(elem);
            this->read_require(num);
            std::memcpy(elem, this->read_data(), num);
            this->read_off += num;
        }

        ///@}

        /**
         * @name Put Functions
         * Higher level functions for writing / putting data into the buffer.
         */
        ///@{
#pragma mark Put Functions

        /**
         * @brief Convenience function to ensure at least `num` bytes are available to write to.
         * @throws buffer_too_small if there are not enough bytes left.
         * @param num 
         */
        void write_require(size_t num) const
        {
            if (this->write_size() < num) {
                throw buffer_too_small(this->write_size());
            }
        }

        /**
         * @brief Put one byte into the buffer and increment the current write head.
         * @throws buffer_too_small if there are no more bytes left to write.
         * @param val 
         */
        void put_byte(u8 val)
        {
            if (this->write_size() < 1) {
                throw buffer_too_small(max_size);
            }
            this->storage.at(this->write_off) = val;
            this->write_off++;
        }

        /**
         * @brief Append the data inside `buf` to our buffer and increment the write head accordingly.
         * @note In contrast to `read_buf()` etc., this does not make a copy of the buf object.
            It does however, copy the contents governed by `buf` into our underlying storage.
         * @throws buffer_too_small if there are not enough bytes left.
         * @tparam TBuffer 
         * @param buf 
         */
        template<typename TBuffer>
        void append_buf(TBuffer& buf) requires Bufferable<TBuffer>
        {
            this->write_require(buf.size());
            std::memcpy(this->write_data(), buf.data(), buf.size());
            this->write_off += buf.size();
        }

        /**
         * @brief Convenience function to advance the write head by `num` bytes.
         * Used in tandem with the low level functions (e.g. `write_buf()`) that do not automatically increase the read head.
         * @throws buffer_too_small if there are not enough bytes left.
         * 
         * @param num 
         */
        void put_count(size_t num)
        {
            // TODO: Kinda too late at this point lmao
            this->write_require(num);
            this->write_off += num;
        }

        /**
         * @brief Similarly to `read_into()`, this writes `sizeof(T)` from `*elem` into our buffer and advances the write head accordingly.
         * @throws buffer_too_small if there are not enough bytes left.
         * @tparam T 
         * @param elem 
         */
        template<typename T>
        void write_outof(T* elem)
        {
            size_t num = sizeof(T);
            this->write_require(num);
            std::memcpy(this->write_data(), elem, num);
            this->write_off += num;
        }

        ///@}

        /**
         * @name Miscellaneous Functions
         * Functions that have not found a good home anywhere else
         */
        ///@{
#pragma mark Miscellaneous Functions

        /**
         * @brief Resets the buffer to the initial state, i.e. `read_size() == 0 && write_size() == max_size`.
         @note Does not do any memsetting, so old data is still stored in the underlying storage.
         * 
         */
        void reset()
        {
            this->write_off = 0;
            this->read_off = 0;
        }

        ///@}

    private:
        /**
         * @brief The maximum allowed size our buffer can have.
         * 
         * Since this is constant, the size of the buffer is fixed at construction time.
         */
        const size_t max_size = 0;

        /**
         * @brief The current offset into `storage`, where we can write new data.
         * 
         */
        size_t write_off = 0;

        /**
         * @brief The current offset into `storage`, where we can read from.
         * @note This does not mean, there is actually any readable data available. Always check `read_size()` or use a higher level function.
         * 
         */
        size_t read_off = 0;

        /**
         * @brief The underlying storage where the actual buffer contents are stored.
         *
         * @note Originally, this was an `std::array` and hence the buffer's size was determined at compile time.
         * This was however rather annoying, as any functions taking a buffer as an argument had to have a template parameter for the size.
         * Therefore, it was deemed easier to just use a vector that is only resized once.
         * 
         */
        std::vector<u8> storage;
    };

} // namespace tasarch::gdb


#endif /* __GDB_BUFFER_H */