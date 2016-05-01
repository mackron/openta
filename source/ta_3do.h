// Public domain. See "unlicense" statement at the end of this file.

typedef struct
{
    // The name of the object.
    const char* name;

} ta_3do_object;

typedef struct
{
    // A pointer to the file to load from.
    ta_file* pFile;

} ta_3do;

// Opens a 3DO object from a file.
ta_3do* ta_open_3do(ta_fs* pFS, const char* fileName);

// Closes the given 3DO object.
void ta_close_3do(ta_3do* p3DO);