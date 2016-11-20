// Copyright (C) 2016 David Reid. See included LICENSE file.

// GAF files are just a collection of relatively small images. Often they are used in animations, but they
// are also used more generically for things like icons and textures.

typedef struct
{
    // The name of the entry.
    char name[32];

    // The number of frames in the entry.
    uint32_t frameCount;

} ta_gaf_entry;

struct ta_gaf
{
    // The name of the file as specified by ta_open_gaf().
    char filename[TA_MAX_PATH];

    // The file to load from.
    ta_file* pFile;

    // The number of entries making up the GAF archive.
    uint32_t entryCount;

    // Internal use only. The position in the file of the list of frame pointers for the current entry.
    uint32_t _entryPointer;

    // Internal use only. The number of frames in the currently selected entry.
    uint32_t _entryFrameCount;

};

// Opens a GAF archive.
ta_gaf* ta_open_gaf(ta_fs* pFS, const char* filename);

// Closes the given GAF archive.
void ta_close_gaf(ta_gaf* pGAF);

// Selects the entry with the given name. After calling this you can get information about each frame in an entry.
ta_bool32 ta_gaf_select_entry(ta_gaf* pGAF, const char* entryName, uint32_t* pFrameCountOut);

// Retrieves the image data of the frame at the given index of the currently selected entry. Free the returned
// pointer with ta_gaf_free().
ta_result ta_gaf_get_frame(ta_gaf* pGAF, uint32_t frameIndex, uint32_t* pWidthOut, uint32_t* pHeightOut, int32_t* pPosXOut, int32_t* pPosYOut, ta_uint8** ppImageData);

// Frees a buffer returned by ta_gaf_get_frame().
void ta_gaf_free(void* pBuffer);