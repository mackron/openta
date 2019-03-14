// Copyright (C) 2018 David Reid. See included LICENSE file.

taString taMallocString(size_t sizeInBytesIncludingNullTerminator)
{
    if (sizeInBytesIncludingNullTerminator == 0) {
        return NULL;
    }

    return (taString)calloc(sizeInBytesIncludingNullTerminator, 1);   // Use calloc() to ensure it's null terminated.
}

taString taMakeString(const char* str)
{
    if (str == NULL) return NULL;
    
    size_t len = strlen(str);
    char* newStr = (char*)malloc(len+1);
    if (newStr == NULL) {
        return NULL;    // Out of memory.
    }

    ta_strcpy_s(newStr, len+1, str);
    return newStr;
}

taString taMakeStringv(const char* format, va_list args)
{
    if (format == NULL) format = "";

    va_list args2;
    va_copy(args2, args);

#if defined(_MSC_VER)
    int len = _vscprintf(format, args2);
#else
    int len = vsnprintf(NULL, 0, format, args2);
#endif

    va_end(args2);
    if (len < 0) {
        return NULL;
    }


    char* str = (char*)malloc(len+1);
    if (str == NULL) {
        return NULL;
    }

#if defined(_MSC_VER)
    len = vsprintf_s(str, len+1, format, args);
#else
    len = vsnprintf(str, len+1, format, args);
#endif

    return str;
}

taString taMakeStringf(const char* format, ...)
{
    if (format == NULL) format = "";

    va_list args;
    va_start(args, format);

    char* str = taMakeStringv(format, args);

    va_end(args);
    return str;
}

taString taSetString(taString str, const char* newStr)
{
    if (newStr == NULL) newStr = "";

    if (str == NULL) {
        return taMakeString(newStr);
    } else {
        // If there's enough room for the new string don't bother reallocating.
        size_t oldStrCap = taStringCapacity(str);
        size_t newStrLen = strlen(newStr);

        if (oldStrCap < newStrLen) {
            str = (taString)realloc(str, newStrLen + 1);  // +1 for null terminator.
            if (str == NULL) {
                return NULL;    // Out of memory.
            }
        }

        memcpy(str, newStr, newStrLen+1);   // +1 to include the null terminator.
        return str;
    }
}

taString taAppendString(taString lstr, const char* rstr)
{
    if (rstr == NULL) {
        rstr = "";
    }

    if (lstr == NULL) {
        return taMakeString(rstr);
    }

    size_t lstrLen = strlen(lstr);
    size_t rstrLen = strlen(rstr);
    char* str = (char*)realloc(lstr, lstrLen + rstrLen + 1);
    if (str == NULL) {
        return NULL;
    }

    memcpy(str + lstrLen, rstr, rstrLen);
    str[lstrLen + rstrLen] = '\0';

    return str;
}

taString taAppendStringv(taString lstr, const char* format, va_list args)
{
    taString rstr = taMakeStringv(format, args);
    if (rstr == NULL) {
        return NULL;    // Probably out of memory.
    }

    char* str = taAppendString(lstr, rstr);

    taFreeString(rstr);
    return str;
}

taString taAppendStringf(taString lstr, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char* str = taAppendStringv(lstr, format, args);

    va_end(args);
    return str;
}

taString taAppendStringLength(taString lstr, const char* rstr, size_t rstrLen)
{
    if (rstr == NULL) {
        rstr = "";
    }

    if (lstr == NULL) {
        return taMakeString(rstr);
    }

    size_t lstrLen = taStringLength(lstr);
    char* str = (char*)realloc(lstr, lstrLen + rstrLen + 1);
    if (str == NULL) {
        return NULL;
    }

    ta_strncat_s(str, lstrLen + rstrLen + 1, rstr, rstrLen);
    str[lstrLen + rstrLen] = '\0';

    return str;
}

size_t taStringLength(taString str)
{
    return strlen(str);
}

size_t taStringCapacity(taString str)
{
    // Currently we're not doing anything fancy with the memory management of strings, but this API is used right now
    // so that future optimizations are easily enabled.
    return taStringLength(str);
}

void taFreeString(taString str)
{
    free(str);
}