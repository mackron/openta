// Copyright (C) 2018 David Reid. See included LICENSE file.

char* ta_make_stringv(const char* format, va_list args)
{
    if (format == NULL) return NULL;

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

char* ta_make_stringf(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char* str = ta_make_stringv(format, args);

    va_end(args);
    return str;
}

void ta_free_string(char* str)
{
    free(str);
}