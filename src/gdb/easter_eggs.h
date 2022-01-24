#ifndef __EASTER_EGGS_H
#define __EASTER_EGGS_H

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include <fmt/core.h>
#include "util/defines.h"

namespace tasarch::gdb {
    struct internal_buf
    {
        size_t address;
        size_t len;
    };

    struct internal_mem
    {
        const static size_t address = 0x1337'1337'1337'0000;

        static constexpr auto start() -> size_t
        {
            return address;
        }

        static constexpr auto get_addr(size_t idx) -> size_t
        {
            return start() + idx;
        }

        static auto end() -> size_t
        {
            return start() + storage.size();
        }

        static auto has_addr(size_t addr) -> bool
        {
            return (addr >= start()) && (addr < end());
        }

        static auto get_idx(size_t addr) -> size_t
        {
            if (!has_addr(addr)) {
                throw std::out_of_range(fmt::format("Address 0x{:x} is out of range of internal debugger memory, [0x{:x}, 0x{:x}].", addr, start(), end()));
            }

            return addr - start();
        }

        static auto read_data(size_t addr, size_t len) -> std::vector<u8>
        {
            size_t idx = get_idx(addr);
            size_t end_idx = idx + len;
            end_idx = std::min(end_idx, storage.size());
            return std::vector<u8>(storage.begin() + idx, storage.begin() + end_idx);
        }

        static auto write_data(size_t addr, size_t len, std::vector<u8>& data) -> void
        {
            size_t idx = get_idx(addr);
            size_t end_idx = idx + len;
            end_idx = std::min(end_idx, storage.size());
            size_t max_len = end_idx - idx;
            std::copy(data.begin(), data.begin() + max_len, storage.begin() + idx);
        }

        static auto add_data(std::vector<u8>& data) -> internal_buf
        {
            auto inserted = storage.insert(storage.end(), data.begin(), data.end());
            size_t idx = std::distance(storage.begin(), inserted);
            return internal_buf {get_addr(idx), data.size()};
        }

        static auto add_str(std::string s) noexcept -> internal_buf
        {
            auto inserted = storage.insert(storage.end(), s.begin(), s.end());
            // for null byte!
            reserve(1);
            size_t idx = std::distance(storage.begin(), inserted);
            return internal_buf {get_addr(idx), s.size()};
        }

        static auto reserve(size_t num) -> internal_buf
        {
            std::vector<u8> data;
            data.resize(num);
            return add_data(data);
        }

        inline static internal_buf static_buf;
        inline static internal_buf echo_hacked;

        inline static void init()
        {
            if (did_init) {
                return;
            }
            did_init = true;
            static_buf = reserve(0x1000);
            echo_hacked = add_str("echo 'hacked';");
        }

    private:
        inline static std::vector<u8> storage;
        inline static bool did_init = false;

    };
} // namespace tasarch::gdb

#endif /* __EASTER_EGGS_H */