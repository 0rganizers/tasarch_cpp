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
        get_val = 1 << 0,
        set_val = 1 << 1
    };

    inline auto operator|(query_type a, query_type b) -> query_type
    {
        return static_cast<query_type>(static_cast<int>(a) | static_cast<int>(b));
    }
} // namespace tasarch::gdb

#endif /* __GDB_PROTOCOL_H */