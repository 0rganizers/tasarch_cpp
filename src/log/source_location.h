#ifndef __SOURCE_LOCATION_H
#define __SOURCE_LOCATION_H

/**
    @file source_location.h
    @brief Wrapper around source_location, to make sure it is included / even there in case of clang!
    We also ensure that the correct header is included, by checking defines. Some compilers still have it in experimental.
 */

#ifdef __cpp_lib_source_location
#include <source_location>
using source_location = std::source_location;
#elifdef __cpp_lib_experimental_source_location
#include <experimental/source_location>
using source_location = std::experimental::source_location;
#else

// straight up stolen from gcc lul
// basically banking on the intrinsics being implemented, which is actually the case since clang 9+???
// Why the fuck then did they not add this snippet below????
// Anyways, wondering if msvc actually has this implemented (they should according to cppreference)
namespace tasarch::log {
    /**
     Theoretically, C++20 has the new and shiny source_location stuff. Unfortunately, it is not equally supported by all compilers.
     I think gcc trunk, has it out of experimental, msvc as well?
     Apple clang, does not have it. However, Apple clang does have the necessary builtins (for some reason) already.
     So we can just copy the source_location definition from gcc's github and call it a day :)
     */
    struct source_location
      {
      private:
        using uint_least32_t = unsigned;
      public:

        // 14.1.2, source_location creation
        static constexpr source_location
        current(const char* __file = __builtin_FILE(),
            const char* __func = __builtin_FUNCTION(),
            int __line = __builtin_LINE(),
            int __col = 0) noexcept
        {
          source_location __loc;
          __loc._M_file = __file;
          __loc._M_func = __func;
          __loc._M_line = static_cast<uint_least32_t>(__line);
          __loc._M_col = static_cast<uint_least32_t>(__col);
          return __loc;
        }

        constexpr source_location() noexcept
        : _M_file("unknown"), _M_func(_M_file), _M_line(0), _M_col(0)
        { }

        // 14.1.3, source_location field access
        constexpr uint_least32_t line() const noexcept { return _M_line; }
        constexpr uint_least32_t column() const noexcept { return _M_col; }
        constexpr const char* file_name() const noexcept { return _M_file; }
        constexpr const char* function_name() const noexcept { return _M_func; }

      private:
        const char* _M_file;
        const char* _M_func;
        uint_least32_t _M_line;
        uint_least32_t _M_col;
      };
}

#endif /* __cpp_lib_source_location */
#endif /* __SOURCE_LOCATION_H */
