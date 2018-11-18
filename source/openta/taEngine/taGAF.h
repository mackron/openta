// Copyright (C) 2018 David Reid. See included LICENSE file.

// GAF files are just a collection of relatively small images. Often they are used in animations, but they
// are also used more generically for things like icons and textures.

typedef struct
{
    // The name of the file as specified by ta_open_gaf().
    char filename[TA_MAX_PATH];

    // The file to load from.
    ta_file* pFile;

    // The number of sequences making up the GAF archive.
    taUInt32 sequenceCount;

    // Internal use only. The name of the currently selected sequence.
    const char* _sequenceName;

    // Internal use only. The position in the file of the list of frame pointers for the current sequence.
    taUInt32 _sequencePointer;

    // Internal use only. The number of frames in the currently selected sequence.
    taUInt32 _sequenceFrameCount;
} ta_gaf;

// Opens a GAF archive.
ta_gaf* ta_open_gaf(ta_fs* pFS, const char* filename);

// Closes the given GAF archive.
void ta_close_gaf(ta_gaf* pGAF);

// Selects the sequence with the given name. After calling this you can get information about each frame in a sequence.
taBool32 ta_gaf_select_sequence(ta_gaf* pGAF, const char* sequenceName, taUInt32* pFrameCountOut);
taBool32 ta_gaf_select_sequence_by_index(ta_gaf* pGAF, taUInt32 index, taUInt32* pFrameCountOut);

// Retrieves the image data of the frame at the given index of the currently selected sequence. Free the returned
// pointer with ta_gaf_free().
taResult ta_gaf_get_frame(ta_gaf* pGAF, taUInt32 frameIndex, taUInt32* pWidthOut, taUInt32* pHeightOut, taInt32* pPosXOut, taInt32* pPosYOut, taUInt8** ppImageData);

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
	taUInt32 atlasIndex;
	taUInt32 localFrameIndex;
	taUInt32 sequenceIndex;
} ta_gaf_texture_group_frame;

typedef struct
{
	char* name;
	taUInt32 firstFrameIndex;
	taUInt32 frameCount;
} ta_gaf_texture_group_sequence;

typedef struct
{
	taEngineContext* pEngine;
    taTexture** ppAtlases;
    taUInt32 atlasCount;
	ta_gaf_texture_group_sequence* pSequences;
    taUInt32 sequenceCount;
	ta_gaf_texture_group_frame* pFrames;
    taUInt32 frameCount;
	taUInt8* _pPayload;
} ta_gaf_texture_group;

taResult ta_gaf_texture_group_init(taEngineContext* pEngine, const char* filePath, ta_color_mode colorMode, ta_gaf_texture_group* pGroup);
taResult ta_gaf_texture_group_uninit(ta_gaf_texture_group* pGroup);
taBool32 ta_gaf_texture_group_find_sequence_by_name(ta_gaf_texture_group* pGroup, const char* sequenceName, taUInt32* pSequenceIndex);