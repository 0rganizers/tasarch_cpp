#ifndef __LITERALS_H
#define __LITERALS_H

#include "defines.h"

namespace tasarch {
inline namespace literals {

    constexpr ALWAYS_INLINE u64 operator ""_KB(unsigned long long n) {
        return static_cast<u64>(n) * UINT64_C(1024);
    }

    constexpr ALWAYS_INLINE u64 operator ""_MB(unsigned long long n) {
        return operator ""_KB(n) * UINT64_C(1024);
    }

    constexpr ALWAYS_INLINE u64 operator ""_GB(unsigned long long n) {
        return operator ""_MB(n) * UINT64_C(1024);
    }

} // namespace literals 
} // namespace tasarch

#endif /* __LITERALS_H */
