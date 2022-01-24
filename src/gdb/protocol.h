#ifndef __GDB_PROTOCOL_H
#define __GDB_PROTOCOL_H

#include "util/defines.h"

namespace tasarch::gdb {
    enum packet_type : u8
	{
		enable_extended = '!',
		query_stop_reason = '?',
		cont = 'c',
		cont_sig = 'C',
		detach = 'D',
		file_io = 'F',
		read_gpr = 'g',
		write_gpr = 'G',
		set_thd = 'H',
		step_cycle = 'i',
		step_cycle_sig = 'I',
		kill = 'k',
		read_mem = 'm',
		write_mem = 'M',
		read_reg = 'p',
		write_reg = 'P',
		get = 'q',
		set = 'Q',
		reset = 'r',
		restart = 'R',
		step = 's',
		step_sig = 'S',
		search = 't',
		query_thd = 'T',
		vcont = 'v',
		write_mem_bin = 'X',
		add_break = 'z',
		del_break = 'Z'
	};

    enum query_type
    {
		none = 0 << 0,
        get_val = 1 << 0,
        set_val = 1 << 1
    };

    inline auto operator|(query_type a, query_type b) -> query_type
    {
        return static_cast<query_type>(static_cast<int>(a) | static_cast<int>(b));
    }

	enum remote_io_errno
	{
		remio_EPERM           = 1,
		remio_ENOENT          = 2,
		remio_EINTR           = 4,
		remio_EBADF           = 9,
		remio_EACCES         = 13,
		remio_EFAULT         = 14,
		remio_EBUSY          = 16,
		remio_EEXIST         = 17,
		remio_ENODEV         = 19,
		remio_ENOTDIR        = 20,
		remio_EISDIR         = 21,
		remio_EINVAL         = 22,
		remio_ENFILE         = 23,
		remio_EMFILE         = 24,
		remio_EFBIG          = 27,
		remio_ENOSPC         = 28,
		remio_ESPIPE         = 29,
		remio_EROFS          = 30,
		remio_ENAMETOOLONG   = 91,
		remio_EUNKNOWN       = 9999
	};
} // namespace tasarch::gdb

#endif /* __GDB_PROTOCOL_H */