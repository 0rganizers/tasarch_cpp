#ifndef __LITERALS_H
#define __LITERALS_H

#include "defines.h"

namespace tasarch {
/**
 * @brief Handy collections of literals that can be useful throught the codebase :)
 */
inline namespace literals {

    /**
     * @brief (IEC) KiB, so `1024` bytes. Example:
     * @code {.cpp}
     * u64 packet_size = 4_KB;
     * @endcode
     */
    constexpr ALWAYS_INLINE u64 operator ""_KB(unsigned long long n) {
        return static_cast<u64>(n) * UINT64_C(1024);
    }

    /**
     * @brief (IEC) MiB, so `1024*1024` bytes. Example:
     * @code {.cpp}
     * u64 file_size = 4_MB;
     * @endcode
     */
    constexpr ALWAYS_INLINE u64 operator ""_MB(unsigned long long n) {
        return operator ""_KB(n) * UINT64_C(1024);
    }

    /**
     * @brief (IEC) GiB, so `1024` bytes. Example:
     * @code {.cpp}
     * u64 game_size = 4_GB;
     * @endcode
     */
    constexpr ALWAYS_INLINE u64 operator ""_GB(unsigned long long n) {
        return operator ""_MB(n) * UINT64_C(1024);
    }

} // namespace literals 
} // namespace tasarch

#endif /* __LITERALS_H */
