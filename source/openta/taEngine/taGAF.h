// Copyright (C) 2018 David Reid. See included LICENSE file.

// GAF files are just a collection of relatively small images. Often they are used in animations, but they
// are also used more generically for things like icons and textures.

typedef struct
{
    // The name of the file as specified by taOpenGAF().
    char filename[TA_MAX_PATH];

    // The file to load from.
    taFile* pFile;

    // The number of sequences making up the GAF archive.
    taUInt32 sequenceCount;

    // Internal use only. The name of the currently selected sequence.
    const char* _sequenceName;

    // Internal use only. The position in the file of the list of frame pointers for the current sequence.
    taUInt32 _sequencePointer;

    // Internal use only. The number of frames in the currently selected sequence.
    taUInt32 _sequenceFrameCount;
} taGAF;

// Opens a GAF archive.
taGAF* taOpenGAF(taFS* pFS, const char* filename);

// Closes the given GAF archive.
void taCloseGAF(taGAF* pGAF);

// Selects the sequence with the given name. After calling this you can get information about each frame in a sequence.
taBool32 taGAFSelectSequence(taGAF* pGAF, const char* sequenceName, taUInt32* pFrameCountOut);
taBool32 taGAFSelectSequenceByIndex(taGAF* pGAF, taUInt32 index, taUInt32* pFrameCountOut);

// Retrieves the image data of the frame at the given index of the currently selected sequence. Free the returned
// pointer with taGAFFree().
taResult taGAFGetFrame(taGAF* pGAF, taUInt32 frameIndex, taUInt32* pWidthOut, taUInt32* pHeightOut, taInt32* pPosXOut, taInt32* pPosYOut, taUInt8** ppImageData);

// Retrieves the name of the currently selected sequence.
const char* taGAFGetCurrentSequenceName(taGAF* pGAF);

// Frees a buffer returned by taGAFGetFrame().
void taGAFFree(void* pBuffer);



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
} taGAFTextureGroupFrame;

typedef struct
{
	char* name;
	taUInt32 firstFrameIndex;
	taUInt32 frameCount;
} taGAFTextureGroupSequence;

typedef struct
{
	taEngineContext* pEngine;
    taTexture** ppAtlases;
    taUInt32 atlasCount;
	taGAFTextureGroupSequence* pSequences;
    taUInt32 sequenceCount;
	taGAFTextureGroupFrame* pFrames;
    taUInt32 frameCount;
	taUInt8* _pPayload;
} taGAFTextureGroup;

taResult taGAFTextureGroupInit(taEngineContext* pEngine, const char* filePath, taColorMode colorMode, taGAFTextureGroup* pGroup);
taResult taGAFTextureGroupUninit(taGAFTextureGroup* pGroup);
taBool32 taGAFTextureGroupFindSequenceByName(taGAFTextureGroup* pGroup, const char* sequenceName, taUInt32* pSequenceIndex);
