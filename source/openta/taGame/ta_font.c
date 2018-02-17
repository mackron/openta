// Copyright (C) 2018 David Reid. See included LICENSE file.

#define TA_MAX_FONT_SIZE    64     // <-- Don't make this too big otherwise you'll end up using too much stack space. Can probably improve this later.

ta_result ta_font_load_fnt(ta_game* pGame, const char* filePath, ta_font* pFont)
{
    assert(pGame != NULL);
    assert(filePath != NULL);
    assert(pFont != NULL);

    pFont->canBeColored = TA_TRUE;

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
    ta_texture_packer_init(&packer, atlasSizeX, atlasSizeY, 1, TA_TEXTURE_PACKER_FLAG_TRANSPARENT_EDGE);

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
            pFont->glyphs[i].originX = 0;
            pFont->glyphs[i].originY = 0;
            pFont->glyphs[i].sizeX = widths[i];
            pFont->glyphs[i].sizeY = pFont->height;
        }
    }

    ta_close_file(pFile);

    pFont->pTexture = ta_create_texture(pGame->pGraphics, packer.width, packer.height, 1, packer.pImageData);
    if (pFont->pTexture == NULL) {
        return TA_ERROR;
    }

    return TA_SUCCESS;
}

ta_result ta_font_load_gaf(ta_game* pGame, const char* filePath, ta_font* pFont)
{
    assert(pGame != NULL);
    assert(filePath != NULL);
    assert(pFont != NULL);

    pFont->canBeColored = TA_FALSE;

    // The file path will look like: <SomeFile.GAF>/<Sequence Name>.
    //
    // We need to parse the file path to determine the exact .GAF package and the sequence. The sequence should have 256 frames,
    // with each frame corresponding to an ANSI(?) character.
    //
    // The frame's x and y positions determine the origin of the glyph.
    const char* sequenceName = drpath_file_name(filePath);
    if (ta_is_string_null_or_empty(sequenceName)) {
        return TA_FILE_NOT_FOUND;
    }

    char packagePath[TA_MAX_PATH];
    if (!drpath_copy_and_remove_file_name(packagePath, sizeof(packagePath), filePath)) {
        return TA_FILE_NOT_FOUND;
    }

    ta_gaf* pGAF = ta_open_gaf(pGame->pFS, packagePath);
    if (pGAF == NULL) {
        return TA_FILE_NOT_FOUND;
    }

    ta_uint32 frameCount;
    if (!ta_gaf_select_sequence(pGAF, sequenceName, &frameCount) || frameCount < 256) {
        ta_close_gaf(pGAF);
        return TA_INVALID_RESOURCE;
    }

    // We will decode the font in two passes, with the first pass used to calculate the required size of the texture atlas.
    ta_uint32 totalWidth = 0;
    ta_uint32 totalHeight = 0;
    for (int i = 0; i < 256; ++i) {
        ta_uint32 sizeX;
        ta_uint32 sizeY;
        ta_int32 posX;
        ta_int32 posY;
        if (ta_gaf_get_frame(pGAF, i, &sizeX, &sizeY, &posX, &posY, NULL) == TA_SUCCESS) {
            totalWidth += sizeX;
            if (totalHeight < sizeY) {
                totalHeight = sizeY;
            }

            totalWidth += 1;    // Padding column for handling interpolation at render time.
        }
    }

    pFont->height = (float)totalHeight;

    // The texture atlas does not need to be square, but it should be a power of 2 for maximum
    // compatibility with older hardware. This can be optimized later.
    ta_uint32 atlasSizeX = ta_next_power_of_2(totalWidth);
    ta_uint32 atlasSizeY = ta_next_power_of_2(totalHeight+1);   // Add 1 to ensure we have at least row of padding for interpolation.

    ta_texture_packer packer;
    ta_texture_packer_init(&packer, atlasSizeX, atlasSizeY, 1, TA_TEXTURE_PACKER_FLAG_TRANSPARENT_EDGE);
    memset(packer.pImageData, TA_TRANSPARENT_COLOR, packer.width*packer.height);

    ta_uint8 paddingPixels[TA_MAX_FONT_SIZE];
    memset(paddingPixels, TA_TRANSPARENT_COLOR, sizeof(paddingPixels));

    for (int i = 0; i < 256; ++i) {
        ta_uint32 sizeX;
        ta_uint32 sizeY;
        ta_int32 posX;
        ta_int32 posY;
        ta_uint8* pixels;
        if (ta_gaf_get_frame(pGAF, i, &sizeX, &sizeY, &posX, &posY, &pixels) == TA_SUCCESS) {
            totalWidth += sizeX;
            if (totalHeight < sizeY) {
                totalHeight = sizeY;
            }

            ta_texture_packer_slot slot;

            // Spaces are a special case because the graphic for them is not blank for some reason. To fix this we just
            // need to use a different image for spaces.
            if (i == ' ') {
                ta_uint8* spacePixels = (ta_uint8*)malloc(sizeX * sizeY);
                if (spacePixels != NULL) {
                    memset(spacePixels, TA_TRANSPARENT_COLOR, sizeX * sizeY);
                    ta_texture_packer_pack_subtexture(&packer, sizeX, sizeY, spacePixels, &slot);
                    free(spacePixels);
                }
            } else {
                ta_texture_packer_pack_subtexture(&packer, sizeX, sizeY, pixels, &slot);
            }

            pFont->glyphs[i].u = slot.posX / (float)packer.width;
            pFont->glyphs[i].v = slot.posY / (float)packer.height;
            pFont->glyphs[i].originX = (float)posX;
            pFont->glyphs[i].originY = (float)totalHeight - (float)posY;
            pFont->glyphs[i].sizeX = (float)sizeX;
            pFont->glyphs[i].sizeY = (float)sizeY;

            // We need to add a padding row in between each character.
            ta_texture_packer_pack_subtexture(&packer, 1, totalHeight, paddingPixels, NULL);

            ta_gaf_free(pixels);
        }
    }

    ta_close_gaf(pGAF);

    ta_uint32* pImageDataRGBA = (ta_uint32*)malloc(packer.width * packer.height * 4);
    if (pImageDataRGBA == NULL) {
        ta_texture_packer_uninit(&packer);
        return TA_OUT_OF_MEMORY;
    }

    for (ta_uint32 y = 0; y < packer.height; ++y) {
        for (ta_uint32 x = 0; x < packer.width; ++x) {
            ta_uint32 color = pGame->palette[packer.pImageData[(y*packer.width) + x]];
            pImageDataRGBA[(y*packer.width) + x] = color;
        }
    }

    pFont->pTexture = ta_create_texture(pGame->pGraphics, packer.width, packer.height, 4, pImageDataRGBA);
    if (pFont->pTexture == NULL) {
        free(pImageDataRGBA);
        ta_texture_packer_uninit(&packer);
        return TA_ERROR;
    }

    free(pImageDataRGBA);
    ta_texture_packer_uninit(&packer);
    return TA_SUCCESS;
}

ta_result ta_font_load(ta_game* pGame, const char* filePath, ta_font* pFont)
{
    if (pFont == NULL) return TA_INVALID_ARGS;
    ta_zero_object(pFont);

    if (pGame == NULL || filePath == NULL) return TA_INVALID_ARGS;
    pFont->pGame = pGame;

    // The font is loaded differently depending on whether or not it's being loaded from a .FNT file or a .GAF file.
    if (drpath_extension_equal(filePath, "FNT")) {
        return ta_font_load_fnt(pGame, filePath, pFont);
    } else {
        return ta_font_load_gaf(pGame, filePath, pFont);
    }
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
            currentLineSizeX += pFont->glyphs[c].sizeX*scale;
            if (pSizeY && *pSizeY < pFont->glyphs[c].sizeY*scale) *pSizeY = pFont->glyphs[c].sizeY*scale;
        }
    }

    if (pSizeX && *pSizeX < currentLineSizeX) *pSizeX = currentLineSizeX;
    return TA_SUCCESS;
}

ta_result ta_font_find_character_metrics(ta_font* pFont, float scale, const char* text, char cIn, float* pPosX, float* pPosY, float* pSizeX, float* pSizeY)
{
    if (pPosX) *pPosX = 0;
    if (pPosY) *pPosY = 0;
    if (pSizeX) *pSizeX = 0;
    if (pSizeY) *pSizeY = 0;
    if (pFont == NULL || text == NULL) return TA_INVALID_ARGS;

    float penPosX = 0;
    float penPosY = 0;
    for (;;) {
        char c = *text++;
        if (c == '\0') {
            return TA_ERROR; // Could not find any occurance of the given character
        }

        if (c == '\n') {
            penPosY += pFont->height*scale;
            penPosX  = 0;
        } else {
            if (c == cIn) {
                if (pPosX) *pPosX = penPosX + pFont->glyphs[c].originX*scale;
                if (pPosY) *pPosY = penPosY + pFont->glyphs[c].originY*scale;
                if (pSizeX) *pSizeX = pFont->glyphs[c].sizeX*scale;
                if (pSizeY) *pSizeY = pFont->glyphs[c].sizeY*scale;
                return TA_SUCCESS;
            } else {
                penPosX += pFont->glyphs[c].sizeX*scale;
            }
        }
    }

    return TA_ERROR;
}