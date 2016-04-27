// Public domain. See "unlicense" statement at the end of this file.

// Credits to http://units.tauniverse.com/tutorials/tadesign/tadesign/ta-gaf-fmt.txt for the description of this file format.

// IMPROVEMENTS
//  - Can probably avoid allocating memory for subframes - just build the frame in-place.
//  - Improve atlas packing. Stop using stb_pack_rect - it's API doesn't really suit us.

bool ta_gaf__read_frame(ta_file* pFile, ta_gaf_entry_frame* pFrame, uint32_t* palette)
{
    // This function assumes the file is sitting on the first byte of the frame.

    if (!ta_read_file(pFile, &pFrame->width, 2, NULL)) {
        return false;
    }

    if (!ta_read_file(pFile, &pFrame->height, 2, NULL)) {
        return false;
    }

    if (!ta_read_file(pFile, &pFrame->offsetX, 2, NULL)) {
        return false;
    }

    if (!ta_read_file(pFile, &pFrame->offsetY, 2, NULL)) {
        return false;
    }

    if (!ta_seek_file(pFile, 1, ta_seek_origin_current)) {
        return false;
    }

    uint8_t isCompressed;
    if (!ta_read_file(pFile, &isCompressed, 1, NULL)) {
        return false;
    }

    if (!ta_read_file(pFile, &pFrame->subframeCount, 2, NULL)) {
        return false;
    }

    if (!ta_seek_file(pFile, 4, ta_seek_origin_current)) {
        return false;
    }

    uint32_t frameDataPtr;
    if (!ta_read_file(pFile, &frameDataPtr, 4, NULL)) {
        return false;
    }

    if (!ta_seek_file(pFile, 4, ta_seek_origin_current)) {
        return false;
    }



    // We need to branch depending on whether or not we are loading pixel data or subframes.
    if (pFrame->subframeCount == 0)
    {
        // No subframes. The next data to read is the raw image data.
        if (!ta_seek_file(pFile, frameDataPtr, ta_seek_origin_start)) {
            return false;
        }

        // The image data is one byte per pixel, with that byte being an index into the palette which defines the color.
        pFrame->isTransparent = false;
        pFrame->pImageData = malloc(pFrame->width * pFrame->height);

        if (isCompressed)
        {
            for (unsigned int y = 0; y < pFrame->height; ++y)
            {
                uint16_t rowSize;
                if (!ta_read_file(pFile, &rowSize, 2, NULL)) {
                    free(pFrame->pImageData);
                    return false;
                }

                if (rowSize > 0)
                {
                    unsigned int x = 0;

                    uint16_t bytesProcessed = 0;
                    while (bytesProcessed < rowSize)
                    {
                        uint8_t mask;
                        if (!ta_read_file(pFile, &mask, 1, NULL)) {
                            free(pFrame->pImageData);
                            return false;
                        }

                        if ((mask & 0x01) == 0x01)
                        {
                            // Transparent.
                            uint8_t repeat = (mask >> 1);
                            while (repeat > 0) {
                                pFrame->pImageData[(y*pFrame->width) + x] = TA_TRANSPARENT_COLOR;   // Fully transparent.
                                x += 1;
                                repeat -= 1;
                            }

                            pFrame->isTransparent = true;
                        }
                        else if ((mask & 0x02) == 0x02)
                        {
                            // The next byte is repeated.
                            uint8_t repeat = (mask >> 2) + 1;
                            uint8_t value;
                            if (!ta_read_file(pFile, &value, 1, NULL)) {
                                free(pFrame->pImageData);
                                return false;
                            }

                            if (value == TA_TRANSPARENT_COLOR) {
                                value = 0;
                            }

                            while (repeat > 0) {
                                pFrame->pImageData[(y*pFrame->width) + x] = value;
                                x += 1;
                                repeat -= 1;
                            }

                            bytesProcessed += 1;
                        }
                        else
                        {
                            // The next bytes are verbatim.
                            uint8_t repeat = (mask >> 2) + 1;
                            while (repeat > 0)
                            {
                                uint8_t value;
                                if (!ta_read_file(pFile, &value, 1, NULL)) {
                                    free(pFrame->pImageData);
                                    return false;
                                }

                                if (value == TA_TRANSPARENT_COLOR) {
                                    value = 0;
                                }

                                pFrame->pImageData[(y*pFrame->width) + x] = value;

                                x += 1;
                                repeat -= 1;
                                bytesProcessed += 1;
                            }
                        }

                        bytesProcessed += 1;
                    }
                }
                else
                {
                    // rowSize == 0. Assume the whole row is transparent.
                    for (unsigned int x = 0; x < pFrame->width; ++x) {
                        pFrame->pImageData[(y*pFrame->width) + x] = TA_TRANSPARENT_COLOR;   // Fully transparent.
                    }

                    pFrame->isTransparent = true;
                }
            }
        }
        else
        {
            // Uncompressed.
            for (unsigned int y = 0; y < pFrame->height; ++y)
            {
                uint8_t* pRow = pFrame->pImageData + (y*pFrame->width);

                if (!ta_read_file(pFile, pRow, pFrame->width, NULL)) {
                    free(pFrame->pImageData);
                    return false;
                }

                // TODO: How do we check for transparency?
            }
        }
    }
    else
    {
        // It has subframes.
        pFrame->pSubframes = calloc(pFrame->subframeCount, sizeof(*pFrame->pSubframes));
        if (pFrame->pSubframes == NULL) {
            return false;
        }

        // We should already be sitting at the first subframe, so now we can go ahead and load them.
        for (unsigned short iSubframe = 0; iSubframe < pFrame->subframeCount; ++iSubframe)
        {
            // <frameDataPtr> points to a list of pFrame->subframeCount pointers to frame headers.
            uint32_t framePtrPos = frameDataPtr + (iSubframe * 4);  // 4 = size of the pointer.
            if (!ta_seek_file(pFile, framePtrPos, ta_seek_origin_start)) {
                return false;
            }

            uint32_t frameStartPos;
            if (!ta_read_file(pFile, &frameStartPos, 4, NULL)) {
                return false;
            }

            if (!ta_seek_file(pFile, frameStartPos, ta_seek_origin_start)) {
                return false;
            }

            if (!ta_gaf__read_frame(pFile, pFrame->pSubframes + iSubframe, palette))
            {
                free(pFrame->pSubframes);
                pFrame->pSubframes = NULL;
                pFrame->subframeCount = 0;
                return false;
            }
        }

        // At this point we have the subframes so now we need to composite them into a single frame (this one).
        pFrame->isTransparent = true;
        pFrame->pImageData = malloc(pFrame->width * pFrame->height);
        memset(pFrame->pImageData, TA_TRANSPARENT_COLOR, pFrame->width * pFrame->height);
        
        for (unsigned short iSubframe = 0; iSubframe < pFrame->subframeCount; ++iSubframe)
        {
            unsigned short offsetX   = pFrame->offsetX - pFrame->pSubframes[iSubframe].offsetX;
            unsigned short offsetY   = pFrame->offsetY - pFrame->pSubframes[iSubframe].offsetY;
            unsigned short srcWidth  = pFrame->pSubframes[iSubframe].width;
            unsigned short srcHeight = pFrame->pSubframes[iSubframe].height;

            for (unsigned short srcY = 0; srcY < srcHeight; ++srcY)
            {
                //unsigned short srcYFlipped = srcY;
                //unsigned short dstYFlipped = offsetY + srcY;                


                uint8_t* pDstRow = pFrame->pImageData + (offsetY + srcY * pFrame->width);
                uint8_t* pSrcRow = pFrame->pSubframes[iSubframe].pImageData + (srcY * srcWidth);

                for (unsigned short srcX = 0; srcX < srcWidth; ++srcX)
                {
                    short dstX = offsetX + srcX;

                    if (pSrcRow[srcX] != TA_TRANSPARENT_COLOR) {
                        pDstRow[dstX] = pSrcRow[srcX];
                    }
                }
            }
        }
    }


    return true;
}

ta_gaf* ta_load_gaf_from_file(ta_file* pFile, ta_graphics_context* pGraphics, uint32_t* palette)
{
    if (pFile == NULL) {
        return NULL;
    }

    uint32_t version;
    if (!ta_read_file(pFile, &version, 4, NULL)) {
        return NULL;    
    }

    if (version != 0x0010100) {
        return NULL;    // Not a GAF file.
    }


    uint32_t entryCount;
    if (!ta_read_file(pFile, &entryCount, 4, NULL)) {
        return NULL;
    }

    uint32_t unused;
    if (!ta_read_file(pFile, &unused, 4, NULL)) {
        return NULL;
    }


    uint32_t* pEntryPointers = malloc(entryCount * sizeof(uint32_t));
    if (pEntryPointers == NULL) {
        return NULL;
    }

    if (!ta_read_file(pFile, pEntryPointers, entryCount * sizeof(uint32_t), NULL)) {
        goto on_error;
    }



    ta_gaf* pGAF = calloc(1, sizeof(*pGAF));
    if (pGAF == NULL) {
        goto on_error;
    }

    pGAF->entryCount = entryCount;
    pGAF->pEntries = calloc(entryCount, sizeof(*pGAF->pEntries));
    if (pGAF->pEntries == NULL) {
        goto on_error;
    }

    unsigned int totalWidth  = 0;
    unsigned int totalHeight = 0;
    unsigned int totalFrameCount = 0;

    for (uint32_t iEntry = 0; iEntry < entryCount; ++iEntry)
    {
        // Seek to the start of the entry...
        if (!ta_seek_file(pFile, pEntryPointers[iEntry], ta_seek_origin_start)) {
            goto on_error;
        }

        if (!ta_read_file(pFile, &pGAF->pEntries[iEntry].frameCount, 2, NULL)) {
            goto on_error;
        }

        // Don't care about the next 6 bytes.
        if (!ta_seek_file(pFile, 6, ta_seek_origin_current)) {
            goto on_error;
        }

        if (!ta_read_file(pFile, pGAF->pEntries[iEntry].name, 32, NULL)) {
            goto on_error;
        }


        if (pGAF->pEntries[iEntry].frameCount > 0)
        {
            uint64_t* pFramePointers = malloc(pGAF->pEntries[iEntry].frameCount * sizeof(uint64_t));
            if (pFramePointers == NULL) {
                goto on_error;
            }

            if (!ta_read_file(pFile, pFramePointers, pGAF->pEntries[iEntry].frameCount * sizeof(uint64_t), NULL)) {
                free(pFramePointers);
                goto on_error;
            }

            pGAF->pEntries[iEntry].pFrames = calloc(pGAF->pEntries[iEntry].frameCount, sizeof(ta_gaf_entry_frame));
            if (pGAF->pEntries[iEntry].pFrames == NULL) {
                free(pFramePointers);
                goto on_error;
            }

            for (unsigned short iFrame = 0; iFrame < pGAF->pEntries[iEntry].frameCount; ++iFrame)
            {
                uint32_t framePointer = pFramePointers[iFrame] & 0xFFFFFFFF;

                if (!ta_seek_file(pFile, framePointer, ta_seek_origin_start)) {
                    free(pFramePointers);
                    goto on_error;
                }

                ta_gaf_entry_frame* pFrame = pGAF->pEntries[iEntry].pFrames + iFrame;
                if (!ta_gaf__read_frame(pFile, pFrame, palette)) {
                    free(pFramePointers);
                    goto on_error;
                }


                totalWidth += pFrame->width;
                totalHeight += pFrame->height;
            }

            totalFrameCount += pGAF->pEntries[iEntry].frameCount;


            free(pFramePointers);
        }
    }


    // At this point we should have everything loaded into system memory, but now we need to add them to a texture atlas. To do this we first
    // need to determine how large to make the atlas. If we make it too large, it'll waste too much memory, but we also want to reduce the
    // total number of textures as much as possible.
    //
    // We'll use a mostly trial-and-error system to figure out how big to make the texture atlas. The initial size will be based on the average
    // width and height of every frame of every entry.

    // TEMP: For testing, just use 1024x1024 atlas sizes for now.

    // We use stb_rect_pack for now, but I might simplify this to something simpler and more efficient later on.
    stbrp_node* pNodes = malloc(1024 * sizeof(stbrp_node));
    stbrp_rect* pRects = malloc(totalFrameCount * sizeof(stbrp_rect));
    int i = 0;
    for (unsigned int iEntry = 0; iEntry < pGAF->entryCount; ++iEntry) {
        for (unsigned int iFrame = 0; iFrame < pGAF->pEntries[iEntry].frameCount; ++iFrame) {
            pRects[i].id = i;
            pRects[i].w = pGAF->pEntries[iEntry].pFrames[iFrame].width;
            pRects[i].h = pGAF->pEntries[iEntry].pFrames[iFrame].height;
            i += 1;
        }
    }

    assert(i == totalFrameCount);

    uint8_t* pAtlasImageData = malloc(1024 * 1024);

    int framesRemaining = totalFrameCount;
    while (framesRemaining > 0)
    {
        // Clear the atlas image data to transparent for debugging purposes.
        memset(pAtlasImageData, TA_TRANSPARENT_COLOR, 1024 * 1024);

        stbrp_context context;
        stbrp_init_target(&context, 1024, 1024, pNodes, 1024);
        stbrp_pack_rects(&context, pRects, framesRemaining);

        int packedFrameCount = 0;
        for (int iRect = 0; iRect < framesRemaining; ++iRect)
        {
            if (pRects[iRect].was_packed)
            {
                // The frame was packed so we need to do a few things:
                //  1) Set the position of the frame within the atlas
                //  2) Copy the image data to the atlas's image data
                //  3) Free the image data that's sitting on system memory
                //  4) Remove it from the list of rectangles so it's not included in the next iteration

                i = 0;
                for (unsigned int iEntry = 0; iEntry < pGAF->entryCount; ++iEntry) {
                    for (unsigned int iFrame = 0; iFrame < pGAF->pEntries[iEntry].frameCount; ++iFrame) {
                        if (i == pRects[iRect].id) {
                            int atlasPosX = pRects[iRect].x;
                            int atlasPosY = pRects[iRect].y;
                            
                            pGAF->pEntries[iEntry].pFrames[iFrame].atlasPosX = atlasPosX;
                            pGAF->pEntries[iEntry].pFrames[iFrame].atlasPosY = atlasPosY;

                            unsigned short width  = pGAF->pEntries[iEntry].pFrames[iFrame].width;
                            unsigned short height = pGAF->pEntries[iEntry].pFrames[iFrame].height;

                            for (int y = 0; y < height; ++y) {
                                for (int x = 0; x < width; ++x) {
                                    pAtlasImageData[((atlasPosY + y) * 1024) + (atlasPosX + x)] = pGAF->pEntries[iEntry].pFrames[iFrame].pImageData[(y*width) + x];
                                }
                            }

                            pGAF->pEntries[iEntry].pFrames[iFrame].atlasIndex = pGAF->textureAtlasCount;

                            free(pGAF->pEntries[iEntry].pFrames[iFrame].pImageData);
                            pGAF->pEntries[iEntry].pFrames[iFrame].pImageData = NULL;

                            goto finished_packing_rect;
                        }
                        i += 1;
                    }
                }

            finished_packing_rect:
                packedFrameCount += 1;
            }
        }

        if (packedFrameCount == 0)
        {
            // We weren't able to pack any frames. Should never get here if all is working well.
            free(pRects);
            free(pNodes);
            goto on_error;
        }


        // The final stage is to create the texture on the graphics system and add it to our internal list.
        ta_texture* pAtlas = ta_create_texture(pGraphics, 1024, 1024, 1, pAtlasImageData);
        if (pAtlas == NULL) {
            free(pRects);
            free(pNodes);
            goto on_error;
        }

        
        pGAF->ppTextureAtlases = realloc(pGAF->ppTextureAtlases, sizeof(ta_texture*) * (pGAF->textureAtlasCount + 1));
        if (pGAF->ppTextureAtlases == NULL) {
            free(pRects);
            free(pNodes);
            goto on_error;
        }

        pGAF->ppTextureAtlases[pGAF->textureAtlasCount] = pAtlas;
        pGAF->textureAtlasCount += 1;


        // We need to remove some rectangles from the pRects list to ensure we don't try packing them multiple times.
        for (int iRect = 0; iRect < framesRemaining; /* DO NOTHING */) {
            if (pRects[iRect].was_packed) {
                for (int iRect2 = iRect; iRect2 < framesRemaining-1; ++iRect2) {
                    pRects[iRect2] = pRects[iRect2+1];
                }

                framesRemaining -= 1;
            } else {
                ++iRect;
            }
        }
    }


    free(pAtlasImageData);
    free(pRects);
    free(pNodes);

    free(pEntryPointers);
    return pGAF;

on_error:
    free(pEntryPointers);
    ta_unload_gaf(pGAF);
    return NULL;
}

void ta_unload_gaf(ta_gaf* pGAF)
{
    if (pGAF == NULL) {
        return;
    }

    if (pGAF->pEntries) {
        for (unsigned int iEntry = 0; iEntry < pGAF->entryCount; ++iEntry) {
            if (pGAF->pEntries[iEntry].pFrames) {
                for (unsigned short iFrame = 0; iFrame < pGAF->pEntries[iEntry].frameCount; ++iFrame) {
                    if (pGAF->pEntries[iEntry].pFrames[iFrame].pSubframes) {
                        for (unsigned short iSubframe = 0; iSubframe < pGAF->pEntries[iEntry].pFrames[iFrame].subframeCount; ++iSubframe) {
                            free(pGAF->pEntries[iEntry].pFrames[iFrame].pSubframes[iSubframe].pImageData);
                        }

                        free(pGAF->pEntries[iEntry].pFrames[iFrame].pSubframes);
                    }
  
                    free(pGAF->pEntries[iEntry].pFrames[iFrame].pImageData);
                }

                free(pGAF->pEntries[iEntry].pFrames);
            }
        }

        free(pGAF->pEntries);
    }

    if (pGAF->ppTextureAtlases) {
        for (unsigned int iTextureAtlas = 0; iTextureAtlas < pGAF->textureAtlasCount; ++iTextureAtlas) {
            ta_delete_texture(pGAF->ppTextureAtlases[iTextureAtlas]);
        }

        free(pGAF->ppTextureAtlases);
    }

    
    free(pGAF);
}


ta_gaf_entry* ta_gaf_get_entry_by_name(ta_gaf* pGAF, const char* name)
{
    if (pGAF == NULL || name == NULL) {
        return NULL;
    }

    for (unsigned int iEntry = 0; iEntry < pGAF->entryCount; ++iEntry) {
        if (_stricmp(pGAF->pEntries[iEntry].name, name) == 0) {     // <-- Things in TA seem to be case insensitive.
            return &pGAF->pEntries[iEntry];
        }
    }

    return NULL;
}
