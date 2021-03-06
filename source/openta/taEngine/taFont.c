// Copyright (C) 2018 David Reid. See included LICENSE file.

#define TA_MAX_FONT_SIZE    64     // <-- Don't make this too big otherwise you'll end up using too much stack space. Can probably improve this later.

taResult taFontLoadFNT(taEngineContext* pEngine, const char* filePath, taFont* pFont)
{
    assert(pEngine != NULL);
    assert(filePath != NULL);
    assert(pFont != NULL);

    pFont->canBeColored = TA_TRUE;

    taFile* pFile = taOpenFile(pEngine->pFS, filePath, 0);
    if (pFile == NULL) {
        return TA_FILE_NOT_FOUND;
    }

    taUInt16 height;
    taReadFileUInt16(pFile, &height);
    if (height > TA_MAX_FONT_SIZE) {
        taCloseFile(pFile);
        return TA_ERROR;
    }

    taUInt16 padding;
    taReadFileUInt16(pFile, &padding);

    taUInt16 offsets[256];
    taReadFile(pFile, offsets, sizeof(offsets), NULL);

    pFont->height = (float)height;

    // We will decode the font in two passes, with the first pass used to calculate the required size of the texture atlas.
    taUInt32 totalWidth = 0;
    taUInt8 widths[256];
    for (int i = 0; i < 256; ++i) {
        if (offsets[i] != 0) {
            taSeekFile(pFile, offsets[i], taSeekOriginStart);
            taReadFileUInt8(pFile, &widths[i]);
            totalWidth += widths[i];
        } else {
            widths[i] = 0;
        }
    }

    if (totalWidth > 65535) {
        return TA_ERROR;    /* Too wide. TODO: Make the font atlas square instead of one wide row. */
    }

    
    // The texture atlas does not need to be square, but it should be a power of 2 for maximum
    // compatibility with older hardware. This can be optimized later.
    taUInt16 atlasSizeX = (taUInt16)taNextPowerOf2(totalWidth);
    taUInt16 atlasSizeY = (taUInt16)taNextPowerOf2(height);

    taTexturePacker packer;
    taTexturePackerInit(&packer, atlasSizeX, atlasSizeY, 1, TA_TEXTURE_PACKER_FLAG_TRANSPARENT_EDGE);

    taUInt8 pixels[256*256];

    for (int i = 0; i < 256; ++i) {
        if (offsets[i] != 0) {
            taSeekFile(pFile, offsets[i], taSeekOriginStart);
            taSeekFile(pFile, 1, taSeekOriginCurrent);

            memset(pixels, 0, sizeof(pixels));
            taUInt8* nextPixel = pixels;

            // The next width*height bits is the bitmap for the glyph.
            taUInt32 bits = widths[i]*height;
            taUInt32 bytes = bits / 8;
            if (bits % 8 != 0) {
                bytes += 1;
            }

            for (taUInt32 iByte = 0; iByte < bytes; ++iByte) {
                assert(bits > 0);

                taUInt32 data;
                taReadFile(pFile, &data, 1, NULL);

                for (taUInt8 iBit = 0; iBit < 8 && iBit < bits; ++iBit) {
                    *nextPixel++ = ((data & (1 << (7 - iBit))) >> (7 - iBit)) * 255;
                }

                bits -= 8;  // Might underflow, but it doesn't matter.
            }

            taTexturePackerSlot slot;
            taTexturePackerPackSubTexture(&packer, widths[i], height, pixels, &slot);

            pFont->glyphs[i].u = slot.posX / (float)packer.width;
            pFont->glyphs[i].v = slot.posY / (float)packer.height;
            pFont->glyphs[i].originX = 0;
            pFont->glyphs[i].originY = 0;
            pFont->glyphs[i].sizeX = widths[i];
            pFont->glyphs[i].sizeY = pFont->height;
        }
    }

    taCloseFile(pFile);

    pFont->pTexture = taCreateTexture(pEngine->pGraphics, packer.width, packer.height, 1, packer.pImageData);
    if (pFont->pTexture == NULL) {
        return TA_ERROR;
    }

    return TA_SUCCESS;
}

taResult taFontLoadGAF(taEngineContext* pEngine, const char* filePath, taFont* pFont)
{
    assert(pEngine != NULL);
    assert(filePath != NULL);
    assert(pFont != NULL);

    pFont->canBeColored = TA_FALSE;

    // The file path will look like: <SomeFile.GAF>/<Sequence Name>.
    //
    // We need to parse the file path to determine the exact .GAF package and the sequence. The sequence should have 256 frames,
    // with each frame corresponding to an ANSI(?) character.
    //
    // The frame's x and y positions determine the origin of the glyph.
    const char* sequenceName = taPathFileName(filePath);
    if (taIsStringNullOrEmpty(sequenceName)) {
        return TA_FILE_NOT_FOUND;
    }

    char packagePath[TA_MAX_PATH];
    if (!taPathRemoveFileName(packagePath, sizeof(packagePath), filePath)) {
        return TA_FILE_NOT_FOUND;
    }

    taGAF* pGAF = taOpenGAF(pEngine->pFS, packagePath);
    if (pGAF == NULL) {
        return TA_FILE_NOT_FOUND;
    }

    taUInt32 frameCount;
    if (!taGAFSelectSequence(pGAF, sequenceName, &frameCount) || frameCount < 256) {
        taCloseGAF(pGAF);
        return TA_INVALID_RESOURCE;
    }

    // We will decode the font in two passes, with the first pass used to calculate the required size of the texture atlas.
    taUInt32 totalWidth = 0;
    taUInt32 totalHeight = 0;
    for (int i = 0; i < 256; ++i) {
        taUInt16 sizeX;
        taUInt16 sizeY;
        taInt16 posX;
        taInt16 posY;
        if (taGAFGetFrame(pGAF, i, &sizeX, &sizeY, &posX, &posY, NULL) == TA_SUCCESS) {
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
    taUInt16 atlasSizeX = (taUInt16)taNextPowerOf2(totalWidth);
    taUInt16 atlasSizeY = (taUInt16)taNextPowerOf2(totalHeight+1);   // Add 1 to ensure we have at least row of padding for interpolation.

    taTexturePacker packer;
    taTexturePackerInit(&packer, atlasSizeX, atlasSizeY, 1, TA_TEXTURE_PACKER_FLAG_TRANSPARENT_EDGE);
    memset(packer.pImageData, TA_TRANSPARENT_COLOR, packer.width*packer.height);

    taUInt8 paddingPixels[TA_MAX_FONT_SIZE];
    memset(paddingPixels, TA_TRANSPARENT_COLOR, sizeof(paddingPixels));

    for (int i = 0; i < 256; ++i) {
        taUInt16 sizeX;
        taUInt16 sizeY;
        taInt16 posX;
        taInt16 posY;
        taUInt8* pixels;
        if (taGAFGetFrame(pGAF, i, &sizeX, &sizeY, &posX, &posY, &pixels) == TA_SUCCESS) {
            totalWidth += sizeX;
            if (totalHeight < sizeY) {
                totalHeight = sizeY;
            }

            taTexturePackerSlot slot;

            // Spaces are a special case because the graphic for them is not blank for some reason. To fix this we just
            // need to use a different image for spaces.
            if (i == ' ') {
                taUInt8* spacePixels = (taUInt8*)malloc(sizeX * sizeY);
                if (spacePixels != NULL) {
                    memset(spacePixels, TA_TRANSPARENT_COLOR, sizeX * sizeY);
                    taTexturePackerPackSubTexture(&packer, sizeX, sizeY, spacePixels, &slot);
                    free(spacePixels);
                } else {
                    taZeroObject(&slot);
                }
            } else {
                taTexturePackerPackSubTexture(&packer, sizeX, sizeY, pixels, &slot);
            }

            pFont->glyphs[i].u = slot.posX / (float)packer.width;
            pFont->glyphs[i].v = slot.posY / (float)packer.height;
            pFont->glyphs[i].originX = (float)posX;
            pFont->glyphs[i].originY = (float)totalHeight - (float)posY;
            pFont->glyphs[i].sizeX = (float)sizeX;
            pFont->glyphs[i].sizeY = (float)sizeY;

            // We need to add a padding row in between each character.
            taTexturePackerPackSubTexture(&packer, 1, (taUInt16)totalHeight, paddingPixels, NULL);

            taGAFFree(pixels);
        }
    }

    taCloseGAF(pGAF);

    taUInt32* pImageDataRGBA = (taUInt32*)malloc(packer.width * packer.height * 4);
    if (pImageDataRGBA == NULL) {
        taTexturePackerUninit(&packer);
        return TA_OUT_OF_MEMORY;
    }

    for (taUInt32 y = 0; y < packer.height; ++y) {
        for (taUInt32 x = 0; x < packer.width; ++x) {
            taUInt32 color = pEngine->palette[packer.pImageData[(y*packer.width) + x]];
            pImageDataRGBA[(y*packer.width) + x] = color;
        }
    }

    pFont->pTexture = taCreateTexture(pEngine->pGraphics, packer.width, packer.height, 4, pImageDataRGBA);
    if (pFont->pTexture == NULL) {
        free(pImageDataRGBA);
        taTexturePackerUninit(&packer);
        return TA_ERROR;
    }

    free(pImageDataRGBA);
    taTexturePackerUninit(&packer);
    return TA_SUCCESS;
}

taResult taFontLoad(taEngineContext* pEngine, const char* filePath, taFont* pFont)
{
    if (pFont == NULL) return TA_INVALID_ARGS;
    taZeroObject(pFont);

    if (pEngine == NULL || filePath == NULL) return TA_INVALID_ARGS;
    pFont->pEngine = pEngine;

    // The font is loaded differently depending on whether or not it's being loaded from a .FNT file or a .GAF file.
    if (taPathExtensionEqual(filePath, "FNT")) {
        return taFontLoadFNT(pEngine, filePath, pFont);
    } else {
        return taFontLoadGAF(pEngine, filePath, pFont);
    }
}

taResult taFontUnload(taFont* pFont)
{
    if (pFont == NULL) return TA_INVALID_ARGS;
    taDeleteTexture(pFont->pTexture);

    return TA_SUCCESS;
}

taResult taFontMeasureText(taFont* pFont, float scale, const char* text, float* pSizeX, float* pSizeY)
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
            if (pSizeY) {
                *pSizeY += pFont->height*scale;
            }
            if (pSizeX && *pSizeX < currentLineSizeX) {
                *pSizeX = currentLineSizeX;
            }
            currentLineSizeX = 0;
        } else {
            currentLineSizeX += pFont->glyphs[c].sizeX*scale;
            if (pSizeY && *pSizeY < pFont->glyphs[c].sizeY*scale) {
                *pSizeY = pFont->glyphs[c].sizeY*scale;
            }
        }
    }

    if (pSizeX && *pSizeX < currentLineSizeX) {
        *pSizeX = currentLineSizeX;
    }
    return TA_SUCCESS;
}

taResult taFontFindCharacterMetrics(taFont* pFont, float scale, const char* text, char cIn, float* pPosX, float* pPosY, float* pSizeX, float* pSizeY)
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
}