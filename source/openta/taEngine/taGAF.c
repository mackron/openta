// Copyright (C) 2018 David Reid. See included LICENSE file.
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

taBool32 ta_gaf__read_frame_header(ta_gaf* pGAF, ta_gaf_frame_header* pHeader)
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

taBool32 ta_gaf__read_frame_pixels(ta_gaf* pGAF, ta_gaf_frame_header* pFrameHeader, uint16_t dstWidth, uint16_t dstHeight, uint16_t dstOffsetX, uint16_t dstOffsetY, uint8_t* pDstImageData)
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

    if (!ta_read_file_uint32(pGAF->pFile, &pGAF->sequenceCount)) {
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


taBool32 ta_gaf_select_sequence(ta_gaf* pGAF, const char* sequenceName, uint32_t* pFrameCountOut)
{
    if (pGAF == NULL || sequenceName == NULL || pFrameCountOut == NULL) {
        return TA_FALSE;
    }

    // We need to find the sequence which we do by simply iterating over each one and comparing it's name.
    for (uint32_t iSequence = 0; iSequence < pGAF->sequenceCount; ++iSequence)
    {
        // The sequence pointers are located at byte position 12.
        if (!ta_seek_file(pGAF->pFile, 12 + (iSequence * sizeof(uint32_t)), ta_seek_origin_start)) {
            return TA_FALSE;
        }

        uint32_t sequencePointer;
        if (!ta_read_file_uint32(pGAF->pFile, &sequencePointer)) {
            return TA_FALSE;
        }

        if (!ta_seek_file(pGAF->pFile, sequencePointer, ta_seek_origin_start)) {
            return TA_FALSE;
        }


        uint16_t frameCount;
        if (!ta_read_file_uint16(pGAF->pFile, &frameCount)) {
            return TA_FALSE;
        }

        if (!ta_seek_file(pGAF->pFile, 6, ta_seek_origin_current)) {
            return TA_FALSE;
        }

        // The file will be sitting on the sequence's name, so we just need to compare. If they're not equal just try the next sequence.
        const char* pName = pGAF->pFile->pFileData + ta_tell_file(pGAF->pFile);
        if (_stricmp(pName, sequenceName) == 0)
        {
            // It's the sequence we're looking for.
            pGAF->_sequenceName = pName;
            pGAF->_sequencePointer = sequencePointer;
            pGAF->_sequenceFrameCount = frameCount;

            *pFrameCountOut = frameCount;
            return TA_TRUE;
        }
    }
    
    return TA_FALSE;
}

taBool32 ta_gaf_select_sequence_by_index(ta_gaf* pGAF, taUInt32 index, uint32_t* pFrameCountOut)
{
    if (pFrameCountOut) *pFrameCountOut = 0;
    if (pGAF == NULL || index >= pGAF->sequenceCount) {
        return TA_FALSE;
    }

    // The sequence pointers are located at byte position 12.
    if (!ta_seek_file(pGAF->pFile, 12 + (index * sizeof(uint32_t)), ta_seek_origin_start)) {
        return TA_FALSE;
    }

    uint32_t sequencePointer;
    if (!ta_read_file_uint32(pGAF->pFile, &sequencePointer)) {
        return TA_FALSE;
    }

    if (!ta_seek_file(pGAF->pFile, sequencePointer, ta_seek_origin_start)) {
        return TA_FALSE;
    }


    uint16_t frameCount;
    if (!ta_read_file_uint16(pGAF->pFile, &frameCount)) {
        return TA_FALSE;
    }

    if (!ta_seek_file(pGAF->pFile, 6, ta_seek_origin_current)) {
        return TA_FALSE;
    }

    pGAF->_sequenceName = pGAF->pFile->pFileData + ta_tell_file(pGAF->pFile);
    pGAF->_sequencePointer = sequencePointer;
    pGAF->_sequenceFrameCount = frameCount;

    if (pFrameCountOut) *pFrameCountOut = frameCount;
    return TA_TRUE;
}

ta_result ta_gaf_get_frame(ta_gaf* pGAF, uint32_t frameIndex, uint32_t* pWidthOut, uint32_t* pHeightOut, int32_t* pPosXOut, int32_t* pPosYOut, taUInt8** ppImageData)
{
    if (ppImageData) *ppImageData = NULL;
    if (pGAF == NULL || frameIndex >= pGAF->_sequenceFrameCount || pWidthOut == NULL || pHeightOut == NULL || pPosXOut == NULL || pPosYOut == NULL) {
        return TA_ERROR;
    }

    // Must have an sequence selected.
    if (pGAF->_sequencePointer == 0) {
        return TA_ERROR;
    }


    // Each frame pointer is grouped as a 64-bit value. We want the first 32-bits. The frame pointers start 40 bytes
    // after the beginning of the sequence.
    if (!ta_seek_file(pGAF->pFile, pGAF->_sequencePointer + 40 + (frameIndex * sizeof(uint64_t)), ta_seek_origin_start)) {
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

const char* ta_gaf_get_current_sequence_name(ta_gaf* pGAF)
{
    if (pGAF == NULL) return NULL;
    return pGAF->_sequenceName;
}

void ta_gaf_free(void* pBuffer)
{
    free(pBuffer);
}



// GAF Texture Groups
// ==================
TA_INLINE char* ta_gaf_texture_group__copy_sequence_name(char** ppNextStr, const char* src)
{
    char* pNextStr = *ppNextStr;
    size_t srcLen = strlen(src);
    memcpy(pNextStr, src, srcLen+1);

    *ppNextStr += srcLen+1;
    return pNextStr;
}

ta_result ta_gaf_texture_group__create_texture_atlas(taEngineContext* pEngine, ta_gaf_texture_group* pGroup, ta_texture_packer* pPacker, ta_color_mode colorMode, ta_texture** ppTexture)
{
    assert(pEngine != NULL);
    assert(pGroup != NULL);
    assert(pPacker != NULL);
    assert(ppTexture != NULL);

    if (colorMode == ta_color_mode_palette) {
        *ppTexture = ta_create_texture(pEngine->pGraphics, pPacker->width, pPacker->height, 1, pPacker->pImageData);
        if (*ppTexture == NULL) {
            return TA_FAILED_TO_CREATE_RESOURCE;
        }
    } else {
        taUInt32* pImageData = (taUInt32*)malloc(pPacker->width*pPacker->height*4);
        if (pImageData == NULL) {
            return TA_OUT_OF_MEMORY;
        }

        for (taUInt32 y = 0; y < pPacker->height; ++y) {
            for (taUInt32 x = 0; x < pPacker->width; ++x) {
                pImageData[(y*pPacker->width) + x] = pEngine->palette[pPacker->pImageData[(y*pPacker->width) + x]];
            }
        }

        *ppTexture = ta_create_texture(pEngine->pGraphics, pPacker->width, pPacker->height, 4, pImageData);
        if (*ppTexture == NULL) {
            free(pImageData);
            return TA_FAILED_TO_CREATE_RESOURCE;
        }

        free(pImageData);
    }

    return TA_SUCCESS;
}

ta_result ta_gaf_texture_group_init(taEngineContext* pEngine, const char* filePath, ta_color_mode colorMode, ta_gaf_texture_group* pGroup)
{
    if (pGroup == NULL) return TA_INVALID_ARGS;
    ta_zero_object(pGroup);
    
    pGroup->pEngine = pEngine;

    ta_gaf* pGAF = ta_open_gaf(pEngine->pFS, filePath);
    if (pGAF == NULL) {
        return TA_FILE_NOT_FOUND;
    }


    // We load in two passes. The first pass is used to calculate the size of the memory allocation and the second
    // pass performs the actual loading.

    ta_texture_packer packer;
    ta_texture_packer_init(&packer, TA_MAX_TEXTURE_ATLAS_SIZE, TA_MAX_TEXTURE_ATLAS_SIZE, 1, TA_TEXTURE_PACKER_FLAG_HARD_EDGE);

    // PASS #1
    // =======
    taUInt32 totalSequenceCount = 0;
    taUInt32 totalFrameCount = 0;
    taUInt32 totalAtlasCount = 0;

    size_t payloadSize = 0;
    for (taUInt32 iSequence = 0; iSequence < pGAF->sequenceCount; ++iSequence) {
        payloadSize += sizeof(ta_gaf_texture_group_sequence);
        totalSequenceCount += 1;

        taUInt32 frameCount;
        if (ta_gaf_select_sequence_by_index(pGAF, iSequence, &frameCount)) {
            payloadSize += strlen(ta_gaf_get_current_sequence_name(pGAF))+1;

            for (taUInt32 iFrame = 0; iFrame < frameCount; ++iFrame) {
                payloadSize += sizeof(ta_gaf_texture_group_frame);
                totalFrameCount += 1;

                taUInt32 sizeX;
                taUInt32 sizeY;
                taUInt32 posX;
                taUInt32 posY;
                if (ta_gaf_get_frame(pGAF, iFrame, &sizeX, &sizeY, &posX, &posY, NULL) == TA_SUCCESS) {
                    if (!ta_texture_packer_pack_subtexture(&packer, sizeX, sizeY, NULL, NULL)) {
                        // We failed to pack the subtexture which probably means there's not enough room. We just need to reset this packer and try again.
                        totalAtlasCount += 1;
                        ta_texture_packer_reset(&packer);
                        ta_texture_packer_pack_subtexture(&packer, sizeX, sizeY, NULL, NULL);   // <-- It doesn't really matter if this fails - we'll handle it organically later on in the second pass.
                    }
                }
            }
        }
    }

    // There might be textures still sitting in the packer needing to be uploaded.
    if (!ta_texture_packer_is_empty(&packer)) {
        totalAtlasCount += 1;
    }

    payloadSize += sizeof(ta_texture*) * totalAtlasCount;


    size_t atlasesPayloadOffset       = 0;
    size_t sequencesPayloadOffset     = atlasesPayloadOffset   + (sizeof(ta_texture*)                   * totalAtlasCount);
    size_t framesPayloadOffset        = sequencesPayloadOffset + (sizeof(ta_gaf_texture_group_sequence) * totalSequenceCount);
    size_t sequenceNamesPayloadOffset = framesPayloadOffset    + (sizeof(ta_gaf_texture_group_frame)    * totalFrameCount);

    pGroup->_pPayload = (taUInt8*)calloc(1, payloadSize);
    if (pGroup->_pPayload == NULL) {
        ta_close_gaf(pGAF);
        return TA_OUT_OF_MEMORY;
    }

    pGroup->ppAtlases  = (ta_texture**                  )(pGroup->_pPayload + atlasesPayloadOffset);
    pGroup->pSequences = (ta_gaf_texture_group_sequence*)(pGroup->_pPayload + sequencesPayloadOffset);
    pGroup->pFrames    = (ta_gaf_texture_group_frame*   )(pGroup->_pPayload + framesPayloadOffset);

    pGroup->atlasCount    = totalAtlasCount;
    pGroup->sequenceCount = totalSequenceCount;
    pGroup->frameCount    = totalFrameCount;


    // PASS #2
    // =======
    char* pNextStr = (char*)(pGroup->_pPayload + sequenceNamesPayloadOffset);
    ta_texture_packer_reset(&packer);

    totalSequenceCount = 0;
    totalFrameCount    = 0;
    totalAtlasCount    = 0;

    for (taUInt32 iSequence = 0; iSequence < pGAF->sequenceCount; ++iSequence) {
        taUInt32 frameCount;
        if (ta_gaf_select_sequence_by_index(pGAF, iSequence, &frameCount)) {
            pGroup->pSequences[totalSequenceCount].name            = ta_gaf_texture_group__copy_sequence_name(&pNextStr, ta_gaf_get_current_sequence_name(pGAF));
            pGroup->pSequences[totalSequenceCount].firstFrameIndex = totalFrameCount;
            pGroup->pSequences[totalSequenceCount].frameCount      = frameCount;

            totalSequenceCount += 1;
            for (taUInt32 iFrame = 0; iFrame < frameCount; ++iFrame) {
                taUInt32 sizeX;
                taUInt32 sizeY;
                taUInt32 posX;
                taUInt32 posY;
                taUInt8* pImageData;
                if (ta_gaf_get_frame(pGAF, iFrame, &sizeX, &sizeY, &posX, &posY, &pImageData) == TA_SUCCESS) {
                    ta_texture_packer_slot slot;
                    if (!ta_texture_packer_pack_subtexture(&packer, sizeX, sizeY, pImageData, &slot)) {
                        // We failed to pack the subtexture which probably means there's not enough room. We just need to reset this packer and try again.
                        ta_gaf_texture_group__create_texture_atlas(pEngine, pGroup, &packer, colorMode, &pGroup->ppAtlases[totalAtlasCount]);
                        totalAtlasCount += 1;

                        ta_texture_packer_reset(&packer);
                        ta_texture_packer_pack_subtexture(&packer, sizeX, sizeY, pImageData, &slot);
                    }

                    pGroup->pFrames[totalFrameCount].renderOffsetX   = (float)posX;
                    pGroup->pFrames[totalFrameCount].renderOffsetY   = (float)posY;
                    pGroup->pFrames[totalFrameCount].atlasPosX       = (float)slot.posX;
                    pGroup->pFrames[totalFrameCount].atlasPosY       = (float)slot.posY;
                    pGroup->pFrames[totalFrameCount].sizeX           = (float)slot.width;
                    pGroup->pFrames[totalFrameCount].sizeY           = (float)slot.height;
                    pGroup->pFrames[totalFrameCount].atlasIndex      = totalAtlasCount;
                    pGroup->pFrames[totalFrameCount].localFrameIndex = iFrame;
                    pGroup->pFrames[totalFrameCount].sequenceIndex   = iSequence;
                }

                totalFrameCount += 1;
            }
        }
    }

    // There might be textures still sitting in the packer needing to be uploaded.
    if (!ta_texture_packer_is_empty(&packer)) {
        ta_gaf_texture_group__create_texture_atlas(pEngine, pGroup, &packer, colorMode, &pGroup->ppAtlases[totalAtlasCount]);
        totalAtlasCount += 1;
    }


    return TA_SUCCESS;
}

ta_result ta_gaf_texture_group_uninit(ta_gaf_texture_group* pGroup)
{
    if (pGroup == NULL) return TA_INVALID_ARGS;

    free(pGroup->_pPayload);
    return TA_SUCCESS;
}

taBool32 ta_gaf_texture_group_find_sequence_by_name(ta_gaf_texture_group* pGroup, const char* sequenceName, taUInt32* pSequenceIndex)
{
    if (pSequenceIndex) *pSequenceIndex = (taUInt32)-1;
    if (pGroup == NULL || sequenceName == NULL) return TA_FALSE;

    for (taUInt32 iSequence = 0; iSequence < pGroup->sequenceCount; ++iSequence) {
        if (_stricmp(pGroup->pSequences[iSequence].name, sequenceName) == 0) {
            if (*pSequenceIndex) *pSequenceIndex = iSequence;
            return TA_TRUE;
        }
    }

    return TA_FALSE;
}