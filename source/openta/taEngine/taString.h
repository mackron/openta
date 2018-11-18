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


// Creates a formatted string. Free the string with taFreeString().
char* taMakeStringV(const char* format, va_list args);
char* taMakeStringF(const char* format, ...);

// Frees a string created by taMakeString*()
void taFreeString(char* str);
