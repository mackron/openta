// Copyright (C) 2016 David Reid. See included LICENSE file.

// Determines whether or not the given string is null or empty.
TA_INLINE ta_bool32 ta_is_string_null_or_empty(const char* str)
{
    return str == NULL || str[0] == '\0';
}