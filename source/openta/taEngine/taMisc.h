// Copyright (C) 2018 David Reid. See included LICENSE file.

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

typedef void (* taProc)(void);

//// Endian Management ////
static TA_INLINE taBool32 taIsLittleEndian()
{
    int n = 1;
    return (*(char*)&n) == 1;
}

static TA_INLINE taUInt32 taSwapEndianUInt32(taUInt32 n)
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

static TA_INLINE taUInt32 taBE2Host32(taUInt32 n)
{
#ifdef __linux__
    return be32toh(n);
#else
    if (taIsLittleEndian()) {
        return taSwapEndianUInt32(n);
    }

    return n;
#endif
}



static TA_INLINE taUInt32 taNextPowerOf2(taUInt32 value)
{
    --value;

    value = (value >> 1)  | value;
    value = (value >> 2)  | value;
    value = (value >> 4)  | value;
    value = (value >> 8)  | value;
    value = (value >> 16) | value;
        
    return value + 1;
}


#define taAbs(x) (((x) < 0) ? (-(x)) : (x))


//// Memory Stream ////

typedef struct
{
    // A pointer to the start of the memory buffer.
    char* pData;

    // The size of the data.
    size_t dataSize;

    // The current read position.
    size_t currentReadPos;
} taMemoryStream;

// Creates a new memory stream.
taMemoryStream taCreateMemoryStream(void* pData, size_t dataSize);

// Reads data from a memory stream.
size_t taMemoryStreamRead(taMemoryStream* pStream, void* pDataOut, size_t bytesToRead);

// Reads data from a memory stream, but does not move thre read position.
size_t taMemoryStreamPeek(taMemoryStream* pStream, void* pDataOut, size_t bytesToRead);

// Seeks to the given position.
taBool32 taMemoryStreamSeek(taMemoryStream* pStream, taInt64 bytesToSeek, taSeekOrigin origin);

// A simple helper for retrieving the current read position of a memory stream.
size_t taMemoryStreamTell(taMemoryStream* pStream);

// Helper for writing a uint32 at the current position. This replaces the next 4 bytes of data - it does NOT insert it. This will move
// the read position to past the value. This will fail if the stream is at the end and there is no room to fit the value. The stream
// does not expand.
taBool32 taMemoryStreamWriteUInt32(taMemoryStream* pStream, taUInt32 value);
