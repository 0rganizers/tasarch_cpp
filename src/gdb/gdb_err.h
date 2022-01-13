#ifndef __GDB_ERR_H
#define __GDB_ERR_H

#include <exception>
#include <stdexcept>
#include <fmt/core.h>
#include "util/defines.h"

namespace tasarch::gdb {
    /**
	 * @brief Haha non standardization go brrr
	 * @todo stringize!
	 */
	enum err_code : u8
	{
        // used by gdb everywhere
        unknown = 1,
        // internal buffers used by gdbstub ran out of space!
        buf_too_small = 2,
	};

    class gdb_error : public std::runtime_error
    {
    public:
        explicit gdb_error(err_code code, std::string msg = "") : std::runtime_error(fmt::format("Had error 0x{:02x}: {}", code, msg)), code(code) {}
        err_code code;
    };

    class unknown_request : public std::runtime_error
    {
    public:
        explicit unknown_request(std::string req) : std::runtime_error(fmt::format("Encountered unrecognized request '{}'", req)), request(req) {}
        std::string request;
    };
} // namespace tasarch::gdb

#endif /* __GDB_ERR_H */