// Copyright (C) 2016 David Reid. See included LICENSE file.

// Determines whether or not the given string is null or empty.
TA_INLINE ta_bool32 ta_is_string_null_or_empty(const char* str)
{
    return str == NULL || str[0] == '\0';
}


// Creates a formatted string. Free the string with ta_free_string().
char* ta_make_stringv(const char* format, va_list args);
char* ta_make_stringf(const char* format, ...);

// Frees a string created by ta_make_string*()
void ta_free_string(char* str);