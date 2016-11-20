// Copyright (C) 2016 David Reid. See included LICENSE file.
//
// Credits to http://units.tauniverse.com/tutorials/tadesign/tadesign/ta-gaf-fmt.txt for the description of this file format.

// Note that this structure is not the same layout as the header in the file.
typedef struct
{
    uint16_t width;
    uint16_t height;
    int16_t offsetX;
    int16_t offsetY;
    uint8_t isCompressed;
    uint16_t subframeCount;
    uint32_t dataPtr;
} ta_gaf_frame_header;

ta_bool32 ta_gaf__read_frame_header(ta_gaf* pGAF, ta_gaf_frame_header* pHeader)
{
    assert(pGAF != NULL);
    assert(pHeader != NULL);

    // This function assumes the file is sitting on the first byte of the header.

    if (!ta_read_file_uint16(pGAF->pFile, &pHeader->width)) {
        return TA_FALSE;
    }
    if (!ta_read_file_uint16(pGAF->pFile, &pHeader->height)) {
        return TA_FALSE;
    }
    if (!ta_read_file_uint16(pGAF->pFile, &pHeader->offsetX)) {
        return TA_FALSE;
    }
    if (!ta_read_file_uint16(pGAF->pFile, &pHeader->offsetY)) {
        return TA_FALSE;
    }
    if (!ta_seek_file(pGAF->pFile, 1, ta_seek_origin_current)) {
        return TA_FALSE;
    }
    if (!ta_read_file_uint8(pGAF->pFile, &pHeader->isCompressed)) {
        return TA_FALSE;
    }
    if (!ta_read_file_uint16(pGAF->pFile, &pHeader->subframeCount)) {
        return TA_FALSE;
    }
    if (!ta_seek_file(pGAF->pFile, 4, ta_seek_origin_current)) {
        return TA_FALSE;
    }
    if (!ta_read_file_uint32(pGAF->pFile, &pHeader->dataPtr)) {
        return TA_FALSE;
    }
    if (!ta_seek_file(pGAF->pFile, 4, ta_seek_origin_current)) {
        return TA_FALSE;
    }

    return TA_TRUE;
}

ta_bool32 ta_gaf__read_frame_pixels(ta_gaf* pGAF, ta_gaf_frame_header* pFrameHeader, uint16_t dstWidth, uint16_t dstHeight, uint16_t dstOffsetX, uint16_t dstOffsetY, uint8_t* pDstImageData)
{
    assert(pGAF != NULL);
    assert(pFrameHeader != NULL);
    assert(pDstImageData != NULL);

    if (!ta_seek_file(pGAF->pFile, pFrameHeader->dataPtr, ta_seek_origin_start)) {
        return TA_FALSE;
    }

    uint16_t offsetX = dstOffsetX - pFrameHeader->offsetX;
    uint16_t offsetY = dstOffsetY - pFrameHeader->offsetY;

    if (pFrameHeader->isCompressed)
    {
        for (uint32_t y = 0; y < pFrameHeader->height; ++y)
        {
            uint16_t rowSize;
            if (!ta_read_file_uint16(pGAF->pFile, &rowSize)) {
                return TA_FALSE;
            }

            uint8_t* pDstRow = pDstImageData + ((offsetY + y) * dstWidth);
            if (rowSize > 0)
            {
                unsigned int x = 0;

                uint16_t bytesProcessed = 0;
                while (bytesProcessed < rowSize)
                {
                    uint8_t mask;
                    if (!ta_read_file_uint8(pGAF->pFile, &mask)) {
                        return TA_FALSE;
                    }

                    if ((mask & 0x01) == 0x01)
                    {
                        // Transparent.
                        uint8_t repeat = (mask >> 1);
                        x += repeat;
                    }
                    else if ((mask & 0x02) == 0x02)
                    {
                        // The next byte is repeated.
                        uint8_t repeat = (mask >> 2) + 1;
                        uint8_t value;
                        if (!ta_read_file_uint8(pGAF->pFile, &value)) {
                            return TA_FALSE;
                        }

                        if (value == TA_TRANSPARENT_COLOR) {
                            value = 0;
                        }

                        while (repeat > 0) {
                            pDstRow[offsetX + x] = value;
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
                            if (!ta_read_file_uint8(pGAF->pFile, &value)) {
                                return TA_FALSE;
                            }

                            if (value == TA_TRANSPARENT_COLOR) {
                                value = 0;
                            }

                            pDstRow[offsetX + x] = value;

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
            }
        }
    }
    else
    {
        for (unsigned int y = 0; y < pFrameHeader->height; ++y)
        {
            uint8_t* pDstRow = pDstImageData + ((offsetY + y) * dstWidth);
            if (!ta_read_file(pGAF->pFile, pDstRow + offsetX, pFrameHeader->width, NULL)) {
                return TA_FALSE;
            }
        }
    }


    return TA_TRUE;
}


ta_gaf* ta_open_gaf(ta_fs* pFS, const char* filename)
{
    if (pFS == NULL || filename == NULL) {
        return NULL;
    }

    char fullFileName[TA_MAX_PATH];
    if (dr_strcpy_s(fullFileName, sizeof(fullFileName), filename) != 0) {
        return NULL;
    }

    //if (!drpath_copy_and_append(fullFileName, sizeof(fullFileName), "anims", filename)) {
    //    return NULL;
    //}

    if (!drpath_extension_equal(filename, "gaf")) {
        if (!drpath_append_extension(fullFileName, sizeof(fullFileName), "gaf")) {
            return NULL;
        }
    }

    ta_gaf* pGAF = calloc(1, sizeof(*pGAF));
    if (pGAF == NULL) {
        return NULL;
    }

    if (strcpy_s(pGAF->filename, sizeof(pGAF->filename), filename) != 0) {
        goto on_error;
    }
    
    pGAF->pFile = ta_open_file(pFS, fullFileName, 0);
    if (pGAF->pFile == NULL) {
        goto on_error;
    }
    
    uint32_t version;
    if (!ta_read_file_uint32(pGAF->pFile, &version)) {
        goto on_error;    
    }
    if (version != 0x0010100) {
        goto on_error;    // Not a GAF file.
    }

    if (!ta_read_file_uint32(pGAF->pFile, &pGAF->entryCount)) {
        goto on_error;
    }



    return pGAF;

on_error:
    if (pGAF) {
        if (pGAF->pFile) {
            ta_close_file(pGAF->pFile);
        }

        free(pGAF);
    }
    
    return NULL;
}

void ta_close_gaf(ta_gaf* pGAF)
{
    if (pGAF == NULL) {
        return;
    }

    ta_close_file(pGAF->pFile);
    free(pGAF);
}


ta_bool32 ta_gaf_select_entry(ta_gaf* pGAF, const char* entryName, uint32_t* pFrameCountOut)
{
    if (pGAF == NULL || entryName == NULL || pFrameCountOut == NULL) {
        return TA_FALSE;
    }

    // We need to find the entry which we do by simply iterating over each one and comparing it's name.
    for (uint32_t iEntry = 0; iEntry < pGAF->entryCount; ++iEntry)
    {
        // The entry pointers are located at byte position 12.
        if (!ta_seek_file(pGAF->pFile, 12 + (iEntry * sizeof(uint32_t)), ta_seek_origin_start)) {
            return TA_FALSE;
        }

        uint32_t entryPointer;
        if (!ta_read_file_uint32(pGAF->pFile, &entryPointer)) {
            return TA_FALSE;
        }

        if (!ta_seek_file(pGAF->pFile, entryPointer, ta_seek_origin_start)) {
            return TA_FALSE;
        }


        uint16_t frameCount;
        if (!ta_read_file_uint16(pGAF->pFile, &frameCount)) {
            return TA_FALSE;
        }

        if (!ta_seek_file(pGAF->pFile, 6, ta_seek_origin_current)) {
            return TA_FALSE;
        }

        // The file will be sitting on the entry name, so we just need to compare. If they're not equal just try the next entry.
        if (_stricmp(pGAF->pFile->pFileData + ta_tell_file(pGAF->pFile), entryName) == 0)
        {
            // It's the entry we're looking for.
            pGAF->_entryPointer = entryPointer;
            pGAF->_entryFrameCount = frameCount;

            *pFrameCountOut = frameCount;
            return TA_TRUE;
        }
    }
    
    return TA_FALSE;
}

ta_result ta_gaf_get_frame(ta_gaf* pGAF, uint32_t frameIndex, uint32_t* pWidthOut, uint32_t* pHeightOut, int32_t* pPosXOut, int32_t* pPosYOut, ta_uint8** ppImageData)
{
    if (ppImageData) *ppImageData = NULL;
    if (pGAF == NULL || frameIndex >= pGAF->_entryFrameCount || pWidthOut == NULL || pHeightOut == NULL || pPosXOut == NULL || pPosYOut == NULL) {
        return TA_ERROR;
    }

    // Must have an entry selected.
    if (pGAF->_entryPointer == 0) {
        return TA_ERROR;
    }


    // Each frame pointer is grouped as a 64-bit value. We want the first 32-bits. The frame pointers start 40 bytes
    // after the beginning of the entry.
    if (!ta_seek_file(pGAF->pFile, pGAF->_entryPointer + 40 + (frameIndex * sizeof(uint64_t)), ta_seek_origin_start)) {
        return TA_ERROR;
    }

    uint32_t framePointer;
    if (!ta_read_file_uint32(pGAF->pFile, &framePointer)) {
        return TA_ERROR;
    }

    if (!ta_seek_file(pGAF->pFile, framePointer, ta_seek_origin_start)) {
        return TA_ERROR;
    }

    ta_gaf_frame_header frameHeader;
    if (!ta_gaf__read_frame_header(pGAF, &frameHeader)) {
        return TA_ERROR;
    }

    *pWidthOut = frameHeader.width;
    *pHeightOut = frameHeader.height;
    *pPosXOut = frameHeader.offsetX;
    *pPosYOut = frameHeader.offsetY;
    
    if (ppImageData != NULL) {
        uint8_t* pImageData = malloc(frameHeader.width * frameHeader.height);
        if (pImageData == NULL) {
            return TA_ERROR;
        }

        // It's important to clear the image data to the transparent color due to the way we'll be building the frame.
        memset(pImageData, TA_TRANSPARENT_COLOR, frameHeader.width * frameHeader.height);

        // We need to branch depending on whether or not we are loading pixel data or sub-frames.
        if (frameHeader.subframeCount == 0)
        {
            // It's raw pixel data.
            if (!ta_gaf__read_frame_pixels(pGAF, &frameHeader, frameHeader.width, frameHeader.height, frameHeader.offsetX, frameHeader.offsetY, pImageData)) {
                free(pImageData);
                return TA_ERROR;
            }
        }
        else
        {
            // The frame is made up of a bunch of sub-frames. They need to be combined by simply layering them on top
            // of each other.
            for (unsigned short iSubframe = 0; iSubframe < frameHeader.subframeCount; ++iSubframe)
            {
                // frameHeader.dataPtr points to a list of frameHeader.subframeCount pointers to frame headers.
                if (!ta_seek_file(pGAF->pFile, frameHeader.dataPtr + (iSubframe * 4), ta_seek_origin_start)) {
                    free(pImageData);
                    return TA_ERROR;
                }

                uint32_t subframePointer;
                if (!ta_read_file_uint32(pGAF->pFile, &subframePointer)) {
                    free(pImageData);
                    return TA_FALSE;
                }

                if (!ta_seek_file(pGAF->pFile, subframePointer, ta_seek_origin_start)) {
                    return TA_ERROR;
                }

                ta_gaf_frame_header subframeHeader;
                if (!ta_gaf__read_frame_header(pGAF, &subframeHeader)) {
                    return TA_ERROR;
                }

                if (!ta_gaf__read_frame_pixels(pGAF, &subframeHeader, frameHeader.width, frameHeader.height, frameHeader.offsetX, frameHeader.offsetY, pImageData)) {
                    free(pImageData);
                    return TA_ERROR;
                }
            }
        }

        if (ppImageData) *ppImageData = pImageData;
    }

    return TA_SUCCESS;
}

void ta_gaf_free(void* pBuffer)
{
    free(pBuffer);
}
