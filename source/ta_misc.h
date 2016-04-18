// Public domain. See "unlicense" statement at the end of this file.

// This file just contains miscellaneous stuff that doesn't really fit anywhere.

#ifdef _MSC_VER
#include <intrin.h>     // For _byteswap_ulong and _byteswap_uint64
#endif

#ifdef __linux__
#define _BSD_SOURCE
#include <endian.h>
#endif

#ifdef _MSC_VER
#define TA_INLINE __forceinline
#else
#define TA_INLINE inline
#endif

//// Endian Management ////
static TA_INLINE bool ta__is_little_endian()
{
    int n = 1;
    return (*(char*)&n) == 1;
}

static TA_INLINE uint32_t ta__swap_endian_uint32(uint32_t n)
{
#ifdef _MSC_VER
    return _byteswap_ulong(n);
#elif defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC__ >= 3))
    return __builtin_bswap32(n);
#else
    return ((n & 0xFF000000) >> 24) |
           ((n & 0x00FF0000) >>  8) |
           ((n & 0x0000FF00) <<  8) |
           ((n & 0x000000FF) << 24);
#endif
}

static TA_INLINE uint32_t ta__be2host_32(uint32_t n)
{
#ifdef __linux__
    return be32toh(n);
#else
    if (ta__is_little_endian()) {
        return ta__swap_endian_uint32(n);
    }

    return n;
#endif
}