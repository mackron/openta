// Public domain. See "unlicense" statement at the end of this file.

typedef struct
{
    // The width of the frame.
    uint16_t width;

    // The height of the frame.
    uint16_t height;

    // The offset of the frame on the X axis. This is NOT the position within the texture atlas.
    int16_t offsetX;

    // The offset of the frame on the Y axis. This is NOT the position within the texture atlas.
    int16_t offsetY;

    // The position on the x axis within the texture atlas this frame is located at.
    uint16_t texturePosX;

    // The position on the y axis within the texture atlas this frame is located at.
    uint16_t texturePosY;

    // The index of the texture atlas that the frame's graphic is contained in.
    uint16_t textureIndex;


    //uint16_t pIndexData[4];
    //ta_vertex_p2t2 pVertexData[4];

} ta_map_feature_frame;

typedef struct
{
    // The number of frames making up the sequence.
    uint32_t frameCount;

    // The frames making up the sequence.
    ta_map_feature_frame pFrames[1];

} ta_map_feature_sequence;

typedef struct
{
    // The base feature descriptor.
    ta_feature_desc* pDesc;

    // The name of the feature.
    char name[128];

    
    // The default sequence.
    ta_map_feature_sequence* pSequenceDefault;

    // the sequence to play when the unit burns.
    ta_map_feature_sequence* pSequenceBurn;

    // the sequence to play when the unit is destroyed.
    ta_map_feature_sequence* pSequenceDie;

    // The sequence to play when the feature is reclamated.
    ta_map_feature_sequence* pSequenceReclamate;

    // The sequence to use when drawing the shadow.
    ta_map_feature_sequence* pSequenceShadow;


    // The original index of the feature. Internal use only.
    uint32_t _index;

} ta_map_feature_type;

typedef struct
{
    // A pointer to the descriptor of the map feature.
    ta_map_feature_type* pType;

    // The position of the feature in the world. The exact position the object is drawn on the 
    // vertical axis is determined by the y and z positions.
    float posX;
    float posY;
    float posZ;

    // The current sequence to show when drawing the feature.
    ta_map_feature_sequence* pCurrentSequence;

    // The index of the current frame in the sequence. This is used for for determining which frame to draw at render time.
    uint32_t currentFrameIndex;

} ta_map_feature;


typedef struct
{
    // The index of the texture to use when drawing this mesh.
    uint32_t textureIndex;

    // The number of indices in the index buffer. Should always be a multiple of 4.
    uint32_t indexCount;

    // The offset of the first index in the main index buffer.
    uint32_t indexOffset;

} ta_map_terrain_submesh;

// Structure containing information about a single chunk of terrain. A chunk is a square grouping of
// tiles which make up the graphics of the terrain. Each individual tile is 32x32 pixels. A chunk is
// split up into multiple meshes for rendering purposes.
typedef struct
{
    // The number of meshes making up this chunk.
    uint32_t meshCount;

    // The meshes making up this chunk. When the chunk is rendered, it will iterate over each of these
    // and draw them one-by-one.
    ta_map_terrain_submesh* pMeshes;

} ta_map_terrain_chunk;

// Structure containing information about the terrain of a map. The terrain is static and is sub-divided
// into chunks. Each chunk is then sub-divided further into per-texture pieces for rendering purposes.
typedef struct
{
    // The total number of tiles on each axis.
    uint32_t tileCountX;
    uint32_t tileCountY;

    // The number of chunks on each axis.
    uint32_t chunkCountX;
    uint32_t chunkCountY;

    // The chunks making up the map. These are stored in linear order, row-by-row.
    ta_map_terrain_chunk* pChunks;

    // The mesh containing all of the terrains geometric detail. 
    ta_mesh* pMesh;

} ta_map_terrain;

// Structure representing a running map instance. This will include information about the terrain,
// features, units and anything else making up the game at any given time.
struct ta_map_instance
{
    // The game that owns this map instance.
    ta_game* pGame;


    // The map's terrain.
    ta_map_terrain terrain;


    // The number of textures containing every 2D graphic used in this map instance.
    uint32_t textureCount;

    // The list of textures containing every 2D graphic used in this map instance. Each texture is
    // an atlas containing many sub-textures. Whenever a graphic is loaded it is placed into one of
    // these textures.
    ta_texture** ppTextures;


    // The number of feature types as specified by the TNT file.
    uint32_t featureTypesCount;

    // The list of feature types as specified by the TNT file.
    ta_map_feature_type* pFeatureTypes;


    // The number of features sitting on the map.
    uint32_t featureCount;

    // The list of features sitting on the map.
    ta_map_feature* pFeatures;
};

// Loads a map by it's name.
//
// This will search for "maps/<mapName>.ota" and "maps/<mapName>.tnt" files. If one of these are not present, loading will fail.
ta_map_instance* ta_load_map(ta_game* pGame, const char* mapName);

// Deletes the given map.
void ta_unload_map(ta_map_instance* pMap);