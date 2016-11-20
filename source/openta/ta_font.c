// Copyright (C) 2016 David Reid. See included LICENSE file.

#define TA_MAX_FONT_SIZE    64     // <-- Don't make this too big otherwise you'll end up using too much stack space. Can probably improve this later.

ta_result ta_font_load(ta_game* pGame, const char* filePath, ta_font* pFont)
{
    if (pFont == NULL) return TA_INVALID_ARGS;
    ta_zero_object(pFont);

    if (pGame == NULL || filePath == NULL) return TA_INVALID_ARGS;
    pFont->pGame = pGame;
    
    ta_file* pFile = ta_open_file(pGame->pFS, filePath, 0);
    if (pFile == NULL) {
        return TA_FILE_NOT_FOUND;
    }

    ta_uint16 height;
    ta_read_file_uint16(pFile, &height);
    if (height > TA_MAX_FONT_SIZE) {
        ta_close_file(pFile);
        return TA_ERROR;
    }

    ta_uint16 padding;
    ta_read_file_uint16(pFile, &padding);

    ta_uint16 offsets[256];
    ta_read_file(pFile, offsets, sizeof(offsets), NULL);

    pFont->height = (float)height;

    // We will decode the font in two passes, with the first pass used to calculate the required size of the texture atlas.
    ta_uint32 totalWidth = 0;
    ta_uint8 widths[256];
    for (int i = 0; i < 256; ++i) {
        if (offsets[i] != 0) {
            ta_seek_file(pFile, offsets[i], ta_seek_origin_start);
            ta_read_file_uint8(pFile, &widths[i]);
            totalWidth += widths[i];
        } else {
            widths[i] = 0;
        }
    }

    
    // The texture atlas does not need to be square, but it should be a power of 2 for maximum
    // compatibility with older hardware. This can be optimized later.
    ta_uint32 atlasSizeX = ta_next_power_of_2(totalWidth);
    ta_uint32 atlasSizeY = ta_next_power_of_2(height);

    ta_texture_packer packer;
    ta_texture_packer_init(&packer, atlasSizeX, atlasSizeY, 1);

    ta_uint8 pixels[256*256];

    for (int i = 0; i < 256; ++i) {
        if (offsets[i] != 0) {
            ta_seek_file(pFile, offsets[i], ta_seek_origin_start);
            ta_seek_file(pFile, 1, ta_seek_origin_current);

            memset(pixels, 0, sizeof(pixels));
            ta_uint8* nextPixel = pixels;

            // The next width*height bits is the bitmap for the glyph.
            ta_uint32 bits = widths[i]*height;
            ta_uint32 bytes = bits / 8;
            if (bits % 8 != 0) {
                bytes += 1;
            }

            for (ta_uint32 iByte = 0; iByte < bytes; ++iByte) {
                assert(bits > 0);

                ta_uint32 data;
                ta_read_file(pFile, &data, 1, NULL);

                for (ta_uint8 iBit = 0; iBit < 8 && iBit < bits; ++iBit) {
                    *nextPixel++ = ((data & (1 << (7 - iBit))) >> (7 - iBit)) * 255;
                }

                bits -= 8;  // Might underflow, but it doesn't matter.
            }

            ta_texture_packer_slot slot;
            ta_texture_packer_pack_subtexture(&packer, widths[i], height, pixels, &slot);

            pFont->glyphs[i].u = slot.posX / (float)packer.width;
            pFont->glyphs[i].v = slot.posY / (float)packer.height;
            pFont->glyphs[i].width = widths[i];
        }
    }

    ta_close_file(pFile);

    pFont->pTexture = ta_create_texture(pGame->pGraphics, packer.width, packer.height, 1, packer.pImageData);
    if (pFont->pTexture == NULL) {
        return TA_ERROR;
    }

    return TA_SUCCESS;
}

ta_result ta_font_unload(ta_font* pFont)
{
    if (pFont == NULL) return TA_INVALID_ARGS;
    ta_delete_texture(pFont->pTexture);

    return TA_SUCCESS;
}

ta_result ta_font_measure_text(ta_font* pFont, float scale, const char* text, float* pSizeX, float* pSizeY)
{
    if (pSizeX) *pSizeX = 0;
    if (pSizeY) *pSizeY = 0;
    if (pFont == NULL || text == NULL) return TA_INVALID_ARGS;

    // The height is always at least the height of the font itself at a minimum, event for an empty string.
    if (pSizeY) *pSizeY = pFont->height*scale;

    float currentLineSizeX = 0;
    for (;;) {
        char c = *text++;
        if (c == '\0') {
            break;
        }

        if (c == '\n') {
            if (pSizeY) *pSizeY += pFont->height*scale;
            if (pSizeX && *pSizeX < currentLineSizeX) *pSizeX = currentLineSizeX;
            currentLineSizeX = 0;
        } else {
            currentLineSizeX += pFont->glyphs[c].width*scale;
        }
    }

    if (pSizeX && *pSizeX < currentLineSizeX) *pSizeX = currentLineSizeX;
    return TA_SUCCESS;
}