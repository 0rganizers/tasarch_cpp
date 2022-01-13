#ifndef __UTIL_WARNINGS_H
#define __UTIL_WARNINGS_H

// see https://stackoverflow.com/a/45783809
#define DO_PRAGMA_(x) _Pragma (#x)
#define DO_PRAGMA(x) DO_PRAGMA_(x)

#define PUSH_WARNING_STATE DO_PRAGMA(warnings(push)) \
                           DO_PRAGMA(clang diagnostic push) \
                           DO_PRAGMA(GCC diagnostic push)

#define POP_WARNING_STATE DO_PRAGMA(warnings(pop)) \
                          DO_PRAGMA(clang diagnostic pop) \
                          DO_PRAGMA(GCC diagnostic pop)

#define DISABLE_WARNING(WARNING_NAME) DO_PRAGMA(warnings(disable :  WARNING_NAME##_MSC) ) \
                                      DO_PRAGMA(clang diagnostic ignored WARNING_NAME##_CLANG) \
                                      DO_PRAGMA(GCC diagnostic ignored WARNING_NAME##_GCC)

#define WARN_CAST_QUAL_GCC "-Wcast-qual"
#define WARN_CAST_QUAL_CLANG "-Wcast-qual"

#endif /* __UTIL_WARNINGS_H */