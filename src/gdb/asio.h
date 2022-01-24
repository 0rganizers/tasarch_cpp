/**
 * @file asio.h
 * @author Leonardo Galli (leonardo.galli@bluewin.ch)
 * @brief This file is necessary for clangd to work correctly on linux / systems where we build with non clang.
 * @warning Include this file before including any other asio headers! 
 *
 * The reason this is needed is as follows: When clangd parses files, it acts just like clang.
 * Therefore, the boost headers try a different way of including certain things compared to if it was gcc.
 * However, that does not work, since we are not actually compiling with clang and hence it fails to include all the coroutine stuff.
 * To solve this, do an additional configure preset (see HACKING.md) that uses clang and libc++ (clangs stdlib) instead of gcc.
 * Then point clangd to use that compile_commands.json.
 * Unfortunately, the boost still dont work then, since they need std::result_of (even though they dont on macos, not sure why?).
 * So we have to do define it ourselves here.
 *
 * @version 0.1
 * @date 2022-01-13
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef __GDB_ASIO_H
#define __GDB_ASIO_H

#if defined(__clang__) && defined(__clangd_linux_fix__) && __clangd_linux_fix__
#include <type_traits>
namespace std {
	template <typename> struct result_of;
	template <typename F, typename... Args>
	struct result_of<F(Args...)> : std::invoke_result<F, Args...> {};
}
#endif /* defined(__clang__) && __clangd_linux_fix__ */

#include <asio.hpp>

#endif /* __GDB_ASIO_H */