// Copyright (C) 2018 David Reid. See included LICENSE file.

// Determines whether or not the given string is null or empty.
TA_INLINE taBool32 taIsStringNullOrEmpty(const char* str)
{
    return str == NULL || str[0] == '\0';
}

// Special implementation of strlen() which returns 0 if the string is null.
TA_INLINE size_t taStrlenOrZero(const char* str)
{
    if (str == NULL) return 0;
    return strlen(str);
}


#ifndef _MSC_VER
    #ifndef _TRUNCATE
    #define _TRUNCATE ((size_t)-1)
    #endif
#endif

TA_INLINE char* ta_strcpy(char* dst, const char* src)
{
    if (dst == NULL) return NULL;

    // If the source string is null, just pretend it's an empty string. I don't believe this is standard behaviour of strcpy(), but I prefer it.
    if (src == NULL) {
        src = "\0";
    }

#ifdef _MSC_VER
    while (*dst++ = *src++);
    return dst;
#else
    return strcpy(dst, src);
#endif
}

TA_INLINE int ta_strcpy_s(char* dst, size_t dstSizeInBytes, const char* src)
{
#ifdef _MSC_VER
    return strcpy_s(dst, dstSizeInBytes, src);
#else
    if (dst == 0) {
        return EINVAL;
    }
    if (dstSizeInBytes == 0) {
        return ERANGE;
    }
    if (src == 0) {
        dst[0] = '\0';
        return EINVAL;
    }

    size_t i;
    for (i = 0; i < dstSizeInBytes && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }

    if (i < dstSizeInBytes) {
        dst[i] = '\0';
        return 0;
    }

    dst[0] = '\0';
    return ERANGE;
#endif
}

TA_INLINE int ta_strncpy_s(char* dst, size_t dstSizeInBytes, const char* src, size_t count)
{
#ifdef _MSC_VER
    return strncpy_s(dst, dstSizeInBytes, src, count);
#else
    if (dst == 0) {
        return EINVAL;
    }
    if (dstSizeInBytes == 0) {
        return EINVAL;
    }
    if (src == 0) {
        dst[0] = '\0';
        return EINVAL;
    }

    size_t maxcount = count;
    if (count == ((size_t)-1) || count >= dstSizeInBytes) {        // -1 = _TRUNCATE
        maxcount = dstSizeInBytes - 1;
    }

    size_t i;
    for (i = 0; i < maxcount && src[i] != '\0'; ++i) {
        dst[i] = src[i];
    }

    if (src[i] == '\0' || i == count || count == ((size_t)-1)) {
        dst[i] = '\0';
        return 0;
    }

    dst[0] = '\0';
    return ERANGE;
#endif
}

TA_INLINE int ta_strcat_s(char* dst, size_t dstSizeInBytes, const char* src)
{
#ifdef _MSC_VER
    return strcat_s(dst, dstSizeInBytes, src);
#else
    if (dst == 0) {
        return EINVAL;
    }
    if (dstSizeInBytes == 0) {
        return ERANGE;
    }
    if (src == 0) {
        dst[0] = '\0';
        return EINVAL;
    }

    char* dstorig = dst;

    while (dstSizeInBytes > 0 && dst[0] != '\0') {
        dst += 1;
        dstSizeInBytes -= 1;
    }

    if (dstSizeInBytes == 0) {
        return EINVAL;  // Unterminated.
    }


    while (dstSizeInBytes > 0 && src[0] != '\0') {
        *dst++ = *src++;
        dstSizeInBytes -= 1;
    }

    if (dstSizeInBytes > 0) {
        dst[0] = '\0';
    } else {
        dstorig[0] = '\0';
        return ERANGE;
    }

    return 0;
#endif
}

TA_INLINE int ta_strncat_s(char* dst, size_t dstSizeInBytes, const char* src, size_t count)
{
#ifdef _MSC_VER
    return strncat_s(dst, dstSizeInBytes, src, count);
#else
    if (dst == 0) {
        return EINVAL;
    }
    if (dstSizeInBytes == 0) {
        return ERANGE;
    }
    if (src == 0) {
        return EINVAL;
    }

    char* dstorig = dst;

    while (dstSizeInBytes > 0 && dst[0] != '\0') {
        dst += 1;
        dstSizeInBytes -= 1;
    }

    if (dstSizeInBytes == 0) {
        return EINVAL;  // Unterminated.
    }


    if (count == ((size_t)-1)) {        // _TRUNCATE
        count = dstSizeInBytes - 1;
    }

    while (dstSizeInBytes > 0 && src[0] != '\0' && count > 0)
    {
        *dst++ = *src++;
        dstSizeInBytes -= 1;
        count -= 1;
    }

    if (dstSizeInBytes > 0) {
        dst[0] = '\0';
    } else {
        dstorig[0] = '\0';
        return ERANGE;
    }

    return 0;
#endif
}

TA_INLINE size_t ta_strcpy_len(char* dst, size_t dstSize, const char* src)
{
    if (ta_strcpy_s(dst, dstSize, src) == 0) {
        return strlen(dst);
    }

    return 0;
}

TA_INLINE int ta_stricmp(const char* string1, const char* string2)
{
#if defined(_MSC_VER) || defined(__MINGW32__)
    return _stricmp(string1, string2);
#else
    return strcasecmp(string1, string2);
#endif
}


// Converts a UTF-16 character to UTF-32.
TA_INLINE uint32_t taUTF16ToUTF32ch(taUInt16 utf16[2])
{
    if (utf16 == NULL) {
        return 0;
    }

    if (utf16[0] < 0xD800 || utf16[0] > 0xDFFF) {
        return utf16[0];
    } else {
        if ((utf16[0] & 0xFC00) == 0xD800 && (utf16[1] & 0xFC00) == 0xDC00) {
            return ((taUInt32)utf16[0] << 10) + utf16[1] - 0x35FDC00;
        } else {
            return 0;   // Invalid.
        }
    }
}

// Converts a UTF-16 surrogate pair to UTF-32.
TA_INLINE taUInt32 taUTF16PairToUTF32ch(taUInt16 utf160, taUInt16 utf161)
{
    taUInt16 utf16[2];
    utf16[0] = utf160;
    utf16[1] = utf161;
    return taUTF16ToUTF32ch(utf16);
}

// Converts a UTF-32 character to a UTF-16. Returns the number fo UTF-16 values making up the character.
TA_INLINE taUInt32 taUTF32ToUTF16ch(taUInt32 utf32, taUInt16 utf16[2])
{
    if (utf16 == NULL) {
        return 0;
    }

    if (utf32 < 0xD800 || (utf32 >= 0xE000 && utf32 <= 0xFFFF)) {
        utf16[0] = (taUInt16)utf32;
        utf16[1] = 0;
        return 1;
    } else {
        if (utf32 >= 0x10000 && utf32 <= 0x10FFFF) {
            utf16[0] = (taUInt16)(0xD7C0 + (taUInt16)(utf32 >> 10));
            utf16[1] = (taUInt16)(0xDC00 + (taUInt16)(utf32 & 0x3FF));
            return 2;
        } else {
            // Invalid.
            utf16[0] = 0;
            utf16[1] = 0;
            return 0;
        }
    }
}

// Converts a UTF-32 character to a UTF-8 character. Returns the number of bytes making up the UTF-8 character.
TA_INLINE taUInt32 taUTF32ToUTF8ch(taUInt32 utf32, char* utf8, size_t utf8Size)
{
    taUInt32 utf8ByteCount = 0;
    if (utf32 < 0x80) {
        utf8ByteCount = 1;
    } else if (utf32 < 0x800) {
        utf8ByteCount = 2;
    } else if (utf32 < 0x10000) {
        utf8ByteCount = 3;
    } else if (utf32 < 0x110000) {
        utf8ByteCount = 4;
    }

    if (utf8ByteCount > utf8Size) {
        if (utf8 != NULL && utf8Size > 0) {
            utf8[0] = '\0';
        }
        return 0;
    }

    utf8 += utf8ByteCount;
    if (utf8ByteCount < utf8Size) {
        utf8[0] = '\0'; // Null terminate.
    }

    const unsigned char firstByteMark[7] = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
    switch (utf8ByteCount)
    {
        case 4: *--utf8 = (char)((utf32 | 0x80) & 0xBF); utf32 >>= 6;
        case 3: *--utf8 = (char)((utf32 | 0x80) & 0xBF); utf32 >>= 6;
        case 2: *--utf8 = (char)((utf32 | 0x80) & 0xBF); utf32 >>= 6;
        case 1: *--utf8 = (char)(utf32 | firstByteMark[utf8ByteCount]);
        default: break;
    }

    return utf8ByteCount;
}

TA_INLINE taBool32 taIsWhitespace(taUInt32 utf32)
{
    return utf32 == ' ' || utf32 == '\t' || utf32 == '\n' || utf32 == '\v' || utf32 == '\f' || utf32 == '\r';
}

TA_INLINE const char* taLTrim(const char* str)
{
    if (str == NULL) {
        return NULL;
    }

    while (str[0] != '\0' && !(str[0] != ' ' && str[0] != '\t' && str[0] != '\n' && str[0] != '\v' && str[0] != '\f' && str[0] != '\r')) {
        str += 1;
    }

    return str;
}

TA_INLINE const char* taRTrim(const char* str)
{
    if (str == NULL) {
        return NULL;
    }

    const char* rstr = str;
    while (str[0] != '\0') {
        if (taIsWhitespace(str[0])) {
            str += 1;
            continue;
        }

        str += 1;
        rstr = str;
    }

    return rstr;
}

TA_INLINE void taTrim(char* str)
{
    if (str == NULL) {
        return;
    }

    const char* lstr = taLTrim(str);
    const char* rstr = taRTrim(lstr);

    if (lstr > str) {
        memmove(str, lstr, rstr-lstr);
    }

    str[rstr-lstr] = '\0';
}

TA_INLINE void taStringReplaceASCII(char* src, char c, char replacement)
{
    for (;;) {
        if (*src == '\0') {
            break;
        }

        if (*src == c) {
            *src = replacement;
        }

        src += 1;
    }
}



typedef char* taString;

// Allocates the memory for a string, including the null terminator.
//
// Use this API if you want to allocate memory for the string, but you want to fill it with raw data yourself.
taString taMallocString(size_t sizeInBytesIncludingNullTerminator);

// Creates a newly allocated string. Free the string with taFreeString().
taString taMakeString(const char* str);

// Creates a formatted string. Free the string with taFreeString().
taString taMakeStringv(const char* format, va_list args);
taString taMakeStringf(const char* format, ...);

// Changes the value of a string.
taString taSetString(taString str, const char* newStr);

// Appends a string to another taString.
//
// This free's "lstr". Use this API like so: "lstr = taAppendString(lstr, rstr)". It works the same way as realloc().
//
// Use taMakeStringf("%s%s", str1, str2) to append to C-style strings together. An optimized solution for this may be implemented in the future.
taString taAppendString(taString lstr, const char* rstr);

// Appends a formatted string to another taString.
taString taAppendStringv(taString lstr, const char* format, va_list args);
taString taAppendStringf(taString lstr, const char* format, ...);

// Same as taAppendString(), except restricts it to a maximum number of characters and does not require the input
// string to be null terminated.
taString taAppendStringLength(taString lstr, const char* rstr, size_t rstrLen);

// Retrieves the length of the given string.
size_t taStringLength(taString str);

// Retrieves the capacity of the buffer containing the data of the given string, not including the null terminator. Add 1
// to the returned value to get the size of the entire buffer.
size_t taStringCapacity(taString str);


// Frees a string created by taMakeString*()
void taFreeString(taString str);