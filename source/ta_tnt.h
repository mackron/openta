// Public domain. See "unlicense" statement at the end of this file.

// TNT files are where map data is stored. It will be combined with an OTA file (which is just a text-based config
// file) to make up the whole map.

typedef struct
{
    float x;
    float y;
    float u;
    float v;
} ta_tnt_mesh_vertex;

struct ta_tnt_mesh
{
    // The number of quads to draw.
    uint32_t quadCount;

    // The vertex and index data, interleaved. Format is P2T2 (4x floats per element).
    ta_tnt_mesh_vertex pVertices[1];
};

// A tile sub-chunk is the part of a chunk that's sent to the renderer. It contains a single mesh with a single texture atlas.
// there are many sub-chunks for each chunk.
typedef struct
{
    // The texture atlas to use when drawing this chunk.
    ta_texture* pTexture;

    // The mesh to draw when drawing this chunk.
    ta_tnt_mesh* pMesh;

} ta_tnt_tile_subchunk;

// Tiles are grouped into chunks of a specific size. Each chunk is the same size within a single TNT object. A chunk is made up
// of a number sub-chunks, with each sub-chunk containing a single mesh, assigned to a single texture atlas. To draw the chunk,
// one must iterate over each sub-chunk and draw the mesh with it's assigned texture atlas.
typedef struct
{
    // The number of sub-chunks making up this chunk.
    uint16_t subchunkCount;

    // A pointer to the list of sub-chunks.
    ta_tnt_tile_subchunk* pSubchunks;

} ta_tnt_tile_chunk;

struct ta_tnt
{
    // The width of the map, in tiles.
    uint32_t width;

    // The height of the map, in tiles.
    uint32_t height;

    // The sea level.
    int32_t seaLevel;


    // The number of texture atlases containing the tile graphics.
    uint16_t textureCount;

    // The list of texture atlases containing the tile graphics.
    ta_texture** ppTextures;


    // The number of chunks on each axis.
    uint32_t chunkCountX;
    uint32_t chunkCountY;

    // The chunks making up the map. These are stored in linear order, row-by-row.
    ta_tnt_tile_chunk* pChunks;

    
    // The width of the minimap.
    uint32_t minimapWidth;

    // The height of the minimap.
    uint32_t minimapHeight;

    // The texture containing the minimap. When drawing the minimap, adjust the texture coordinates based on the size
    // of the minimap.
    ta_texture* pMinimapTexture;
};

// Loads a TNT file.
ta_tnt* ta_load_tnt_from_file(ta_hpi_file* pFile, ta_graphics_context* pGraphics);

// Deletes the given TNT file.
void ta_unload_tnt(ta_tnt* pTNT);