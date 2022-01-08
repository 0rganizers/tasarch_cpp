#ifndef __DEFINES_H
#define __DEFINES_H

#include <cstdint>

/**
 * @todo This is taken from atmosphere source code. Check legality.
 */

#define NON_COPYABLE(cls) \
    cls(const cls&) = delete; \
    cls& operator=(const cls&) = delete

#define NON_MOVEABLE(cls) \
    cls(cls&&) = delete; \
    cls& operator=(cls&&) = delete

#define ALIGNED(algn) __attribute__((aligned(algn)))
#define NORETURN      __attribute__((noreturn))
#define WEAK_SYMBOL   __attribute__((weak))
#define ALWAYS_INLINE_LAMBDA __attribute__((always_inline))
#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NOINLINE      __attribute__((noinline))

#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)

#define BITSIZEOF(x) (sizeof(x) * CHAR_BIT)

#define STRINGIZE(x) STRINGIZE_IMPL(x)
#define STRINGIZE_IMPL(x) #x

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

#endif /* __DEFINES_H */
