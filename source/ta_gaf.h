// Public domain. See "unlicense" statement at the end of this file.

// GAF files are just a collection of relatively small images. Often they are used in animations, but they
// are also used more generically for things like icons and textures. There are a lot of these images, and
// it's not appropriate to create a separate texture for each image. Thus, we instead pack them into an
// atlas.
//
// Loading a GAF file is done in two distinct parts. The first part loads the raw image data into system
// memory. The second part moves the image data from system memory and packs them into an atlas which is
// owned by the graphics system. Once uploaded to the graphics system, the local copy is freed from system
// memory to save resources.

// ISSUES
//
// There are a lot of GAF files that contain just a few tiny images which would result in quite a lot of
// textures needing be created by the graphics system. It would be more efficient to reduce the number of
// texture atlas' by merging those smaller GAF files together. This would require a higher level object
// to act as an arbiter for managing the texture atlas'. Possible object hierarchy:
//
// ta_gaf_library
//   ta_gaf
//     ta_gaf_entry
//       ta_gaf_entry_frame
//
// This system would require higher level management of the creation of libraries. 

typedef struct ta_gaf_entry_frame ta_gaf_entry_frame;
struct ta_gaf_entry_frame
{
    // The width of the frame.
    unsigned short width;

    // The height of the frame.
    unsigned short height;

    // The offset of the frame on the X axis. This is NOT the position within the texture atlas.
    short offsetX;

    // The offset of the frame on the Y axis. This is NOT the position within the texture atlas.
    short offsetY;

    // The position on the x axis within the texture atlas this frame is located at.
    short atlasPosX;

    // The position on the y axis within the texture atlas this frame is located at.
    short atlasPosY;

    // Whether or not the data should be treated as transparent. This is used when rendering.
    bool isTransparent;

    // The number of subframes. Usually 0, but if not the image data will be null.
    unsigned short subframeCount;

    // The list of subframes.
    ta_gaf_entry_frame* pSubframes;

    // A pointer to the image data. This will be freed and set to null after uploading the image data to a texture atlas.
    uint8_t* pImageData;

    // The index of the texture atlas containing the image data of the frame. This is an index in the list of texture atlas' in
    // the ta_gaf object that owns this frame.
    unsigned short atlasIndex;
};

typedef struct
{
    // The name of the entry.
    char name[32];

    // The number of frames making up the entry.
    unsigned short frameCount;

    // The list of frames making up the entry.
    ta_gaf_entry_frame* pFrames;

} ta_gaf_entry;

typedef struct
{
    // The number of entries in the GAF file.
    unsigned int entryCount;

    // The list of entries.
    ta_gaf_entry* pEntries;

    // The number of texture atlas' containing the image data of each entry.
    unsigned int textureAtlasCount;

    // The texture altas' containing the image data of each entry.
    ta_texture** ppTextureAtlases;

} ta_gaf;

#if 0
typedef struct
{
} ta_gaf_library;
#endif


// Loads a GAF file.
ta_gaf* ta_load_gaf_from_file(ta_hpi_file* pFile, ta_graphics_context* pGraphics, uint32_t* palette);

// Unloads the given GAF package.
void ta_unload_gaf(ta_gaf* pGAF);


// Retrieves a pointer to the entry with the given name. Returns NULL if an entry with the given name doesn't exist.
ta_gaf_entry* ta_gaf_get_entry_by_name(ta_gaf* pGAF, const char* name);
