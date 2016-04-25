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



static TA_INLINE uint32_t ta_next_power_of_2(uint32_t value)
{
    --value;

    value = (value >> 1)  | value;
    value = (value >> 2)  | value;
    value = (value >> 4)  | value;
    value = (value >> 8)  | value;
    value = (value >> 16) | value;
        
    return value + 1;
}


//// Memory Stream ////

typedef struct
{
    // A pointer to the start of the memory buffer.
    char* pData;

    // The size of the data.
    size_t dataSize;

    // The current read position.
    size_t currentReadPos;

} ta_memory_stream;

// Creates a new memory stream.
ta_memory_stream ta_create_memory_stream(void* pData, size_t dataSize);

// Reads data from a memory stream.
size_t ta_memory_stream_read(ta_memory_stream* pStream, void* pDataOut, size_t bytesToRead);

// Reads data from a memory stream, but does not move thre read position.
size_t ta_memory_stream_peek(ta_memory_stream* pStream, void* pDataOut, size_t bytesToRead);

// Seeks to the given position.
bool ta_memory_stream_seek(ta_memory_stream* pStream, int64_t bytesToSeek, ta_seek_origin origin);

// A simple helper for retrieving the current read position of a memory stream.
size_t ta_memory_stream_tell(ta_memory_stream* pStream);

// Helper for writing a uint32 at the current position. This replaces the next 4 bytes of data - it does NOT insert it. This will move
// the read position to past the value. This will fail if the stream is at the end and there is no room to fit the value. The stream
// does not expand.
bool ta_memory_stream_write_uint32(ta_memory_stream* pStream, uint32_t value);
