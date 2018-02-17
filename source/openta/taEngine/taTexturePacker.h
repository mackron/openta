// Copyright (C) 2018 David Reid. See included LICENSE file.

// The texture packer is used for packing small textures into a large texture atlases. It's usage is
// pretty simple - you simply allocate a rectangle region, and then commit image data to it. Note
// that texture packers store all of their image data in system memory and are not linked to the
// graphics sub-system in any way - it's your job to create the texture atlas on the graphics side.
//
// This packer is optimized for speed and simplicity over tight packing. Do not use this if you are
// needing to save as much memory as possible. The way it works is very simple: there are two
// "cursors" that indicate where the next image should be placed - one for the x axis and another
// for the y axis. As sub-images are allocated these cursors move. The image will try to be placed
// on the current row, but if it can't fit it will be placed on the next row. If it can't fit
// anywhere the allocation function will simply return an error in which case the application can
// choose to commit the texture atlas to the graphics system and begin a new one.
//
// The number of bytes per pixel can be specified during initialization. Only uncompressed image
// formats are supported. Indeed, the packer does not care about the specific format of the image
// data, only that there's a constant number of bytes-per-pixel.
//
// Another issue with texture packing is how the edges are handled with respect to linear filering
// and precision errors with UV coordinates. One was of handling this is to put an extra pixel
// around each sub-texture which can be used to emulate clamping. To enable this, set the
// TA_TEXTURE_PACKER_FLAG_HARD_EDGE option flag.

#define TA_TEXTURE_PACKER_FLAG_HARD_EDGE        (1 << 0)
#define TA_TEXTURE_PACKER_FLAG_TRANSPARENT_EDGE (1 << 1)

typedef struct
{
    // The current width of the texture atlas. This is constant.
    uint32_t width;

    // The current height of the texture atlas. This is constant.
    uint32_t height;

    // The number of bytes per pixel.
    uint32_t bpp;

    // Option flags: TA_TEXTURE_PACKER_FLAG_*
    ta_uint32 flags;


    // The cursor position on the x axis. This is where the next image will try to be placed.
    uint32_t cursorPosX;
    uint32_t cursorPosY;

    // The height of the current row.
    uint32_t currentRowHeight;


    // The buffer containing the packed image data.
    uint8_t* pImageData;

} ta_texture_packer;

typedef struct
{
    uint32_t posX;
    uint32_t posY;
    uint32_t width;
    uint32_t height;
} ta_texture_packer_slot;


// Initializes the given texture packer. The minimum and maximum size should be a power of 2.
ta_bool32 ta_texture_packer_init(ta_texture_packer* pPacker, uint32_t width, uint32_t height, uint32_t bytesPerPixel, ta_uint32 flags);

// Uninitializes the given texture packer.
void ta_texture_packer_uninit(ta_texture_packer* pPacker);

// Efficiently resets the texture packer. If you need to use a different sized packer you will need to uninitialize
// and re-initialize it.
void ta_texture_packer_reset(ta_texture_packer* pPacker);

// Packs a sub-texture into the packer. If there is no room this will simply return TA_FALSE.
ta_bool32 ta_texture_packer_pack_subtexture(ta_texture_packer* pPacker, uint32_t width, uint32_t height, const void* pSubTextureData, ta_texture_packer_slot* pSlotOut);

// Determines if the texture packer is empty or not.
ta_bool32 ta_texture_packer_is_empty(const ta_texture_packer* pPacker);
