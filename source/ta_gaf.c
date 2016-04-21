// Public domain. See "unlicense" statement at the end of this file.

// Credits to http://units.tauniverse.com/tutorials/tadesign/tadesign/ta-gaf-fmt.txt for the description of this file format.

bool ta_gaf__read_frame(ta_hpi_file* pFile, ta_gaf_entry_frame* pFrame, uint32_t* palette, bool flipped)
{
    // This function assumes the file is sitting on the first byte of the frame.

    if (!ta_hpi_read(pFile, &pFrame->width, 2, NULL)) {
        return false;
    }

    if (!ta_hpi_read(pFile, &pFrame->height, 2, NULL)) {
        return false;
    }

    if (!ta_hpi_read(pFile, &pFrame->offsetX, 2, NULL)) {
        return false;
    }

    if (!ta_hpi_read(pFile, &pFrame->offsetY, 2, NULL)) {
        return false;
    }

    if (!ta_hpi_seek(pFile, 1, ta_hpi_seek_origin_current)) {
        return false;
    }

    uint8_t isCompressed;
    if (!ta_hpi_read(pFile, &isCompressed, 1, NULL)) {
        return false;
    }

    if (!ta_hpi_read(pFile, &pFrame->subframeCount, 2, NULL)) {
        return false;
    }

    if (!ta_hpi_seek(pFile, 4, ta_hpi_seek_origin_current)) {
        return false;
    }

    uint32_t frameDataPtr;
    if (!ta_hpi_read(pFile, &frameDataPtr, 4, NULL)) {
        return false;
    }

    if (!ta_hpi_seek(pFile, 4, ta_hpi_seek_origin_current)) {
        return false;
    }



    // We need to branch depending on whether or not we are loading pixel data or subframes.
    if (pFrame->subframeCount == 0)
    {
        // No subframes. The next data to read is the raw image data.
        if (!ta_hpi_seek(pFile, frameDataPtr, ta_hpi_seek_origin_start)) {
            return false;
        }

        // The image data is one byte per pixel, with that byte being an index into the palette which defines the color.
        pFrame->isTransparent = false;
        pFrame->pImageData = malloc(pFrame->width * pFrame->height * 4);

        uint32_t* pImageData32 = pFrame->pImageData;

        // TODO: Handle uncompressed data.
        if (isCompressed)
        {
            for (unsigned int y = 0; y < pFrame->height; ++y)
            {
                uint16_t rowSize;
                if (!ta_hpi_read(pFile, &rowSize, 2, NULL)) {
                    free(pImageData32);
                    return false;
                }

                unsigned int yflipped = y;
                if (flipped) {
                    yflipped = (pFrame->height - y - 1);
                }

                if (rowSize > 0)
                {
                    unsigned int x = 0;

                    uint16_t bytesProcessed = 0;
                    while (bytesProcessed < rowSize)
                    {
                        uint8_t mask;
                        if (!ta_hpi_read(pFile, &mask, 1, NULL)) {
                            free(pImageData32);
                            return false;
                        }

                        if ((mask & 0x01) == 0x01)
                        {
                            // Transparent.
                            uint8_t repeat = (mask >> 1);
                            while (repeat > 0) {
                                pImageData32[(yflipped*pFrame->width) + x] = 0x00000000;   // Fully transparent.
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
                            if (!ta_hpi_read(pFile, &value, 1, NULL)) {
                                free(pImageData32);
                                return false;
                            }

                            while (repeat > 0) {
                                pImageData32[(yflipped*pFrame->width) + x] = palette[value] | 0xFF000000;
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
                                if (!ta_hpi_read(pFile, &value, 1, NULL)) {
                                    free(pImageData32);
                                    return false;
                                }

                                pImageData32[(yflipped*pFrame->width) + x] = palette[value] | 0xFF000000;

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
                        pImageData32[(yflipped*pFrame->width) + x] = 0x00000000;   // Fully transparent.
                    }

                    pFrame->isTransparent = true;
                }

                //assert(x == pFrame->width);
            }
        }
        else
        {
            // Uncompressed.
            for (unsigned int y = 0; y < pFrame->height; ++y)
            {
                unsigned int yflipped = y;
                if (flipped) {
                    yflipped = (pFrame->height - y - 1);
                }

                for (unsigned int x = 0; x < pFrame->width; ++x)
                {

                }
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
            if (!ta_hpi_seek(pFile, framePtrPos, ta_hpi_seek_origin_start)) {
                return false;
            }

            uint32_t frameStartPos;
            if (!ta_hpi_read(pFile, &frameStartPos, 4, NULL)) {
                return false;
            }

            if (!ta_hpi_seek(pFile, frameStartPos, ta_hpi_seek_origin_start)) {
                return false;
            }

            if (!ta_gaf__read_frame(pFile, pFrame->pSubframes + iSubframe, palette, flipped))
            {
                free(pFrame->pSubframes);
                pFrame->pSubframes = NULL;
                pFrame->subframeCount = 0;
                return false;
            }
        }
    }


    return true;
}

ta_gaf* ta_load_gaf_from_file(ta_hpi_file* pFile, ta_graphics_context* pGraphics, uint32_t* palette, bool flipped)
{
    if (pFile == NULL) {
        return NULL;
    }

    uint32_t version;
    if (!ta_hpi_read(pFile, &version, 4, NULL)) {
        return NULL;    
    }

    if (version != 0x0010100) {
        return NULL;    // Not a GAF file.
    }


    uint32_t entryCount;
    if (!ta_hpi_read(pFile, &entryCount, 4, NULL)) {
        return NULL;
    }

    uint32_t unused;
    if (!ta_hpi_read(pFile, &unused, 4, NULL)) {
        return NULL;
    }


    uint32_t* pEntryPointers = malloc(entryCount * sizeof(uint32_t));
    if (pEntryPointers == NULL) {
        return NULL;
    }

    if (!ta_hpi_read(pFile, pEntryPointers, entryCount * sizeof(uint32_t), NULL)) {
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

    for (uint32_t iEntry = 0; iEntry < entryCount; ++iEntry)
    {
        // Seek to the start of the entry...
        if (!ta_hpi_seek(pFile, pEntryPointers[iEntry], ta_hpi_seek_origin_start)) {
            goto on_error;
        }

        if (!ta_hpi_read(pFile, &pGAF->pEntries[iEntry].frameCount, 2, NULL)) {
            goto on_error;
        }

        // Don't care about the next 6 bytes.
        if (!ta_hpi_seek(pFile, 6, ta_hpi_seek_origin_current)) {
            goto on_error;
        }

        if (!ta_hpi_read(pFile, pGAF->pEntries[iEntry].name, 32, NULL)) {
            goto on_error;
        }


        if (pGAF->pEntries[iEntry].frameCount > 0)
        {
            uint64_t* pFramePointers = malloc(pGAF->pEntries[iEntry].frameCount * sizeof(uint64_t));
            if (pFramePointers == NULL) {
                goto on_error;
            }

            if (!ta_hpi_read(pFile, pFramePointers, pGAF->pEntries[iEntry].frameCount * sizeof(uint64_t), NULL)) {
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

                if (!ta_hpi_seek(pFile, framePointer, ta_hpi_seek_origin_start)) {
                    free(pFramePointers);
                    goto on_error;
                }

                if (!ta_gaf__read_frame(pFile, &pGAF->pEntries[iEntry].pFrames[iFrame], palette, flipped)) {
                    free(pFramePointers);
                    goto on_error;
                }
            }


            free(pFramePointers);
        }
    }

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

    if (pGAF->pTextureAtlases) {
        for (unsigned int iTextureAtlas = 0; iTextureAtlas < pGAF->textureAtlasCount; ++iTextureAtlas) {
            ta_delete_texture(pGAF->pTextureAtlases[iTextureAtlas]);
        }
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
