// Copyright (C) 2016 David Reid. See included LICENSE file.

// GAF files are just a collection of relatively small images. Often they are used in animations, but they
// are also used more generically for things like icons and textures.

struct ta_gaf
{
    // The name of the file as specified by ta_open_gaf().
    char filename[TA_MAX_PATH];

    // The file to load from.
    ta_file* pFile;

    // The number of sequences making up the GAF archive.
    uint32_t sequenceCount;

    // Internal use only. The name of the currently selected sequence.
    const char* _sequenceName;

    // Internal use only. The position in the file of the list of frame pointers for the current sequence.
    uint32_t _sequencePointer;

    // Internal use only. The number of frames in the currently selected sequence.
    uint32_t _sequenceFrameCount;
};

// Opens a GAF archive.
ta_gaf* ta_open_gaf(ta_fs* pFS, const char* filename);

// Closes the given GAF archive.
void ta_close_gaf(ta_gaf* pGAF);

// Selects the sequence with the given name. After calling this you can get information about each frame in a sequence.
ta_bool32 ta_gaf_select_sequence(ta_gaf* pGAF, const char* sequenceName, uint32_t* pFrameCountOut);
ta_bool32 ta_gaf_select_sequence_by_index(ta_gaf* pGAF, ta_uint32 index, uint32_t* pFrameCountOut);

// Retrieves the image data of the frame at the given index of the currently selected sequence. Free the returned
// pointer with ta_gaf_free().
ta_result ta_gaf_get_frame(ta_gaf* pGAF, uint32_t frameIndex, uint32_t* pWidthOut, uint32_t* pHeightOut, int32_t* pPosXOut, int32_t* pPosYOut, ta_uint8** ppImageData);

// Retrieves the name of the currently selected sequence.
const char* ta_gaf_get_current_sequence_name(ta_gaf* pGAF);

// Frees a buffer returned by ta_gaf_get_frame().
void ta_gaf_free(void* pBuffer);



// GAF Texture Groups
// ==================
typedef struct
{
	float renderOffsetX;
	float renderOffsetY;
	float atlasPosX;
	float atlasPosY;
	float sizeX;
	float sizeY;
	ta_uint32 atlasIndex;
	ta_uint32 localFrameIndex;
	ta_uint32 sequenceIndex;
} ta_gaf_texture_group_frame;

typedef struct
{
	char* name;
	ta_uint32 firstFrameIndex;
	ta_uint32 frameCount;
} ta_gaf_texture_group_sequence;

typedef struct
{
	ta_game* pGame;
    ta_texture** ppAtlases;
    ta_uint32 atlasCount;
	ta_gaf_texture_group_sequence* pSequences;
    ta_uint32 sequenceCount;
	ta_gaf_texture_group_frame* pFrames;
    ta_uint32 frameCount;
	ta_uint8* _pPayload;
} ta_gaf_texture_group;

ta_result ta_gaf_texture_group_init(ta_game* pGame, const char* filePath, ta_gaf_texture_group* pGroup);
ta_result ta_gaf_texture_group_uninit(ta_gaf_texture_group* pGroup);
ta_bool32 ta_gaf_texture_group_find_sequence_by_name(ta_gaf_texture_group* pGroup, const char* sequenceName, ta_uint32* pSequenceIndex);