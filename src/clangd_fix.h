#ifndef __FUCK_CLANGD_H
#define __FUCK_CLANGD_H

// clangd fix
#ifdef __clang__
#include <type_traits>
namespace std {
	template <typename> struct result_of;
	template <typename F, typename... Args>
	struct result_of<F(Args...)> : std::invoke_result<F, Args...> {};
}
#endif
#include <asio.hpp>

#endif /* __FUCK_CLANGD_H */