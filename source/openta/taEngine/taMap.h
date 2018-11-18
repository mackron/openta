// Copyright (C) 2018 David Reid. See included LICENSE file.

typedef struct
{
    // The width of the frame.
    taUInt16 width;

    // The height of the frame.
    taUInt16 height;

    // The offset of the frame on the X axis. This is NOT the position within the texture atlas.
    taInt16 offsetX;

    // The offset of the frame on the Y axis. This is NOT the position within the texture atlas.
    taInt16 offsetY;

    // The position on the x axis within the texture atlas this frame is located at.
    taUInt16 texturePosX;

    // The position on the y axis within the texture atlas this frame is located at.
    taUInt16 texturePosY;

    // The index of the texture atlas that the frame's graphic is contained in.
    taUInt16 textureIndex;
} taMapFeatureFrame;

typedef struct
{
    // The number of frames making up the sequence.
    taUInt32 frameCount;

    // The frames making up the sequence.
    taMapFeatureFrame pFrames[1];
} taMapFeatureSequence;

typedef struct
{
    // The index of the texture to use when drawing the mesh.
    taUInt16 textureIndex;

    // The mesh data.
    taMesh* pMesh;

    // The index count.
    taUInt32 indexCount;

    // The index offset. A mesh can be contained within a larger monolithic mesh. This property keeps track of the position
    // within that mesh where this submesh begins.
    taUInt32 indexOffset;
} taMap3DOMesh;

typedef struct
{
    float relativePosX;
    float relativePosY;
    float relativePosZ;

    // The index of the next sibling.
    taUInt32 nextSiblingIndex;

    // The index of the first child.
    taUInt32 firstChildIndex;

    // The number of meshes making up the object. Meshes are split based on the texture index. 
    size_t meshCount;

    // The index of the first mesh within the main array of the taMap3DO object that owns this.
    size_t firstMeshIndex;
} taMap3DOObject;

typedef struct
{
    // The number of objects making up the 3DO.
    taUInt32 objectCount;
    
    // The list of objects making up the 3DO. The root object is always at index 0.
    taMap3DOObject* pObjects;

    // The number of meshes making up the geometric data of every object. There is usually one mesh per object,
    // but there may be more if an object's mesh data is split over multiple texture atlases.
    taUInt32 meshCount;

    // The list of meshes making up the geometric data of every object. These are grouped per-object.
    taMap3DOMesh* pMeshes;
} taMap3DO;

typedef struct
{
    // The base feature descriptor.
    taFeatureDesc* pDesc;

    // The name of the feature.
    char name[128];

    
    // The default sequence.
    taMapFeatureSequence* pSequenceDefault;

    // the sequence to play when the unit burns.
    taMapFeatureSequence* pSequenceBurn;

    // the sequence to play when the unit is destroyed.
    taMapFeatureSequence* pSequenceDie;

    // The sequence to play when the feature is reclamated.
    taMapFeatureSequence* pSequenceReclamate;

    // The sequence to use when drawing the shadow.
    taMapFeatureSequence* pSequenceShadow;

    // The 3DO object, if it has one. This can be null.
    taMap3DO* p3DO;

    // The original index of the feature. Internal use only.
    taUInt32 _index;
} taMapFeatureType;

typedef struct
{
    // A pointer to the descriptor of the map feature.
    taMapFeatureType* pType;

    // The position of the feature in the world. The exact position the object is drawn on the 
    // vertical axis is determined by the y and z positions.
    float posX;
    float posY;
    float posZ;

    // The current sequence to show when drawing the feature. This will be null if the object is 3D.
    taMapFeatureSequence* pCurrentSequence;

    // The index of the current frame in the sequence. This is used for for determining which frame to draw at render time.
    taUInt32 currentFrameIndex;
} taMapFeature;


typedef struct
{
    // The index of the texture to use when drawing this mesh.
    taUInt32 textureIndex;

    // The number of indices in the index buffer. Should always be a multiple of 4.
    taUInt32 indexCount;

    // The offset of the first index in the main index buffer.
    taUInt32 indexOffset;
} taMapTerrainSubMesh;

// Structure containing information about a single chunk of terrain. A chunk is a square grouping of
// tiles which make up the graphics of the terrain. Each individual tile is 32x32 pixels. A chunk is
// split up into multiple meshes for rendering purposes.
typedef struct
{
    // The number of meshes making up this chunk.
    taUInt32 meshCount;

    // The meshes making up this chunk. When the chunk is rendered, it will iterate over each of these
    // and draw them one-by-one.
    taMapTerrainSubMesh* pMeshes;
} taMapTerrainChunk;

// Structure containing information about the terrain of a map. The terrain is static and is sub-divided
// into chunks. Each chunk is then sub-divided further into per-texture pieces for rendering purposes.
typedef struct
{
    // The total number of tiles on each axis.
    taUInt32 tileCountX;
    taUInt32 tileCountY;

    // The number of chunks on each axis.
    taUInt32 chunkCountX;
    taUInt32 chunkCountY;

    // The chunks making up the map. These are stored in linear order, row-by-row.
    taMapTerrainChunk* pChunks;

    // The mesh containing all of the terrains geometric detail. 
    taMesh* pMesh;
} taMapTerrain;

// Structure representing a running map instance. This will include information about the terrain,
// features, units and anything else making up the game at any given time.
struct taMapInstance
{
    // The engine context that owns this map instance.
    taEngineContext* pEngine;

    // The map's terrain.
    taMapTerrain terrain;


    // The number of textures containing every 2D graphic used in this map instance.
    taUInt32 textureCount;

    // The list of textures containing every 2D graphic used in this map instance. Each texture is
    // an atlas containing many sub-textures. Whenever a graphic is loaded it is placed into one of
    // these textures.
    taTexture** ppTextures;


    // The number of feature types as specified by the TNT file.
    taUInt32 featureTypesCount;

    // The list of feature types as specified by the TNT file.
    taMapFeatureType* pFeatureTypes;


    // The number of features sitting on the map.
    taUInt32 featureCount;

    // The list of features sitting on the map.
    taMapFeature* pFeatures;
};

// Loads a map by it's name.
//
// This will search for "maps/<mapName>.ota" and "maps/<mapName>.tnt" files. If one of these are not present, loading will fail.
taMapInstance* taLoadMap(taEngineContext* pEngine, const char* mapName);

// Deletes the given map.
void taUnloadMap(taMapInstance* pMap);
