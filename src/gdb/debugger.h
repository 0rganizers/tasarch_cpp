#ifndef __DEBUGGER_H
#define __DEBUGGER_H

namespace tasarch::gdb {
	class Debugger  
	{
	public:
		virtual void request_break() = 0;
	};
} // namespace tasarch::gdb

#endif /* __DEBUGGER_H */
