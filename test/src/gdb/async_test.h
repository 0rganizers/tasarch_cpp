#ifndef __ASYNC_TEST_H
#define __ASYNC_TEST_H

#include <ut/ut.hpp>
#include <asio.hpp>

#define throws_async_ex(awaitable, TException) { bool did_throw = false;\
        try { \
            co_await awaitable; \
        } catch (const TException&) { \
            did_throw = true; \
        } catch (...) { \
            did_throw = true; expect(false) << "expected different exception " #TException " to be thrown!"; \
        } \
        if (!did_throw) \
            expect(false) << "expected exception to be thrown!"; \
    }

#define throws_async(awaitable) { bool did_throw = false;\
        try { \
            co_await awaitable; \
        } catch (...) { \
           did_throw = true; \
        } \
        if (!did_throw) \
            expect(false) << "expected exception to be thrown!"; \
    }

/* doesnt work :/
namespace boost::ut {
    namespace detail {
        template <class TExpr, class TException = void>
        struct throws_async_ : op {
        constexpr explicit throws_async_(const asio::awaitable<TExpr>& awaitable)
            : value_{[&awaitable] {
                try {
                    co_await awaitable;
                } catch (const TException&) {
                    co_return true;
                } catch (...) {
                    co_return false;
                }
                co_return false;
                }()} {}

        [[nodiscard]] constexpr operator bool() const { return value_; }

        const bool value_{};
        };

        template <class TExpr>
        struct throws_async_<TExpr, void> : op {
        constexpr explicit throws_async_(const asio::awaitable<TExpr>& awaitable)
            : value_{[&awaitable] {
                try {
                    co_await awaitable;
                } catch (...) {
                    co_return true;
                }
                co_return false;
                }()} {}

        [[nodiscard]] constexpr operator bool() const { return value_; }

        const bool value_{};
        };
    } // namespace detail

    template <class TException, class TExpr>
    [[nodiscard]] constexpr auto throws_async(const asio::awaitable<TExpr>& awaitable) {
        try {
            co_await awaitable;
        } catch (const TException&) {
            expect(true);
        } catch (...) {
            expect(false) << "expected different exception to be thrown!";
        }
        expect(false) << "expected exception to be thrown!";
    }

    template <class TExpr>
    [[nodiscard]] constexpr auto throws_async(const asio::awaitable<TExpr>& awaitable) {
        return detail::throws_async_<TExpr>{awaitable};
    }
} // namespace boost::ut
*/



#endif /* __ASYNC_TEST_H */