// Copyright (C) 2018 David Reid. See included LICENSE file.
//
// Credits to http://units.tauniverse.com/tutorials/tadesign/tadesign/ta-gaf-fmt.txt for the description of this file format.

// Note that this structure is not the same layout as the header in the file.
typedef struct
{
    taUInt16 width;
    taUInt16 height;
    taInt16 offsetX;
    taInt16 offsetY;
    taUInt8 isCompressed;
    taUInt16 subframeCount;
    taUInt32 dataPtr;
} taGAFFrameHeader;

TA_PRIVATE taBool32 taGAFReadFrameHeader(taGAF* pGAF, taGAFFrameHeader* pHeader)
{
    assert(pGAF != NULL);
    assert(pHeader != NULL);

    // This function assumes the file is sitting on the first byte of the header.

    if (!taReadFileUInt16(pGAF->pFile, &pHeader->width)) {
        return TA_FALSE;
    }
    if (!taReadFileUInt16(pGAF->pFile, &pHeader->height)) {
        return TA_FALSE;
    }
    if (!taReadFileInt16(pGAF->pFile, &pHeader->offsetX)) {
        return TA_FALSE;
    }
    if (!taReadFileInt16(pGAF->pFile, &pHeader->offsetY)) {
        return TA_FALSE;
    }
    if (!taSeekFile(pGAF->pFile, 1, taSeekOriginCurrent)) {
        return TA_FALSE;
    }
    if (!taReadFileUInt8(pGAF->pFile, &pHeader->isCompressed)) {
        return TA_FALSE;
    }
    if (!taReadFileUInt16(pGAF->pFile, &pHeader->subframeCount)) {
        return TA_FALSE;
    }
    if (!taSeekFile(pGAF->pFile, 4, taSeekOriginCurrent)) {
        return TA_FALSE;
    }
    if (!taReadFileUInt32(pGAF->pFile, &pHeader->dataPtr)) {
        return TA_FALSE;
    }
    if (!taSeekFile(pGAF->pFile, 4, taSeekOriginCurrent)) {
        return TA_FALSE;
    }

    return TA_TRUE;
}

TA_PRIVATE taBool32 taGAFReadFramePixels(taGAF* pGAF, taGAFFrameHeader* pFrameHeader, taUInt16 dstWidth, taUInt16 dstHeight, taUInt16 dstOffsetX, taUInt16 dstOffsetY, taUInt8* pDstImageData)
{
    assert(pGAF != NULL);
    assert(pFrameHeader != NULL);
    assert(pDstImageData != NULL);

    (void)dstHeight;

    if (!taSeekFile(pGAF->pFile, pFrameHeader->dataPtr, taSeekOriginStart)) {
        return TA_FALSE;
    }

    taUInt16 offsetX = dstOffsetX - pFrameHeader->offsetX;
    taUInt16 offsetY = dstOffsetY - pFrameHeader->offsetY;

    if (pFrameHeader->isCompressed) {
        for (taUInt32 y = 0; y < pFrameHeader->height; ++y) {
            taUInt16 rowSize;
            if (!taReadFileUInt16(pGAF->pFile, &rowSize)) {
                return TA_FALSE;
            }

            taUInt8* pDstRow = pDstImageData + ((offsetY + y) * dstWidth);
            if (rowSize > 0) {
                unsigned int x = 0;

                taUInt16 bytesProcessed = 0;
                while (bytesProcessed < rowSize) {
                    taUInt8 mask;
                    if (!taReadFileUInt8(pGAF->pFile, &mask)) {
                        return TA_FALSE;
                    }

                    if ((mask & 0x01) == 0x01) {
                        // Transparent.
                        taUInt8 repeat = (mask >> 1);
                        x += repeat;
                    } else if ((mask & 0x02) == 0x02) {
                        // The next byte is repeated.
                        taUInt8 repeat = (mask >> 2) + 1;
                        taUInt8 value;
                        if (!taReadFileUInt8(pGAF->pFile, &value)) {
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
                    } else {
                        // The next bytes are verbatim.
                        taUInt8 repeat = (mask >> 2) + 1;
                        while (repeat > 0) {
                            taUInt8 value;
                            if (!taReadFileUInt8(pGAF->pFile, &value)) {
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
            } else {
                // rowSize == 0. Assume the whole row is transparent.
            }
        }
    } else {
        for (unsigned int y = 0; y < pFrameHeader->height; ++y) {
            taUInt8* pDstRow = pDstImageData + ((offsetY + y) * dstWidth);
            if (!taReadFile(pGAF->pFile, pDstRow + offsetX, pFrameHeader->width, NULL)) {
                return TA_FALSE;
            }
        }
    }

    return TA_TRUE;
}


taGAF* taOpenGAF(taFS* pFS, const char* filename)
{
    if (pFS == NULL || filename == NULL) {
        return NULL;
    }

    char fullFileName[TA_MAX_PATH];
    if (ta_strcpy_s(fullFileName, sizeof(fullFileName), filename) != 0) {
        return NULL;
    }

    //if (!taPathAppend(fullFileName, sizeof(fullFileName), "anims", filename)) {
    //    return NULL;
    //}

    if (!taPathExtensionEqual(filename, "gaf")) {
        if (!taPathAppendExtension(fullFileName, sizeof(fullFileName), fullFileName, "gaf")) {
            return NULL;
        }
    }

    taGAF* pGAF = calloc(1, sizeof(*pGAF));
    if (pGAF == NULL) {
        return NULL;
    }

    if (strcpy_s(pGAF->filename, sizeof(pGAF->filename), filename) != 0) {
        goto on_error;
    }
    
    pGAF->pFile = taOpenFile(pFS, fullFileName, 0);
    if (pGAF->pFile == NULL) {
        goto on_error;
    }
    
    taUInt32 version;
    if (!taReadFileUInt32(pGAF->pFile, &version)) {
        goto on_error;    
    }
    if (version != 0x0010100) {
        goto on_error;    // Not a GAF file.
    }

    if (!taReadFileUInt32(pGAF->pFile, &pGAF->sequenceCount)) {
        goto on_error;
    }



    return pGAF;

on_error:
    if (pGAF) {
        if (pGAF->pFile) {
            taCloseFile(pGAF->pFile);
        }

        free(pGAF);
    }
    
    return NULL;
}

void taCloseGAF(taGAF* pGAF)
{
    if (pGAF == NULL) {
        return;
    }

    taCloseFile(pGAF->pFile);
    free(pGAF);
}


taBool32 taGAFSelectSequence(taGAF* pGAF, const char* sequenceName, taUInt32* pFrameCountOut)
{
    if (pGAF == NULL || sequenceName == NULL || pFrameCountOut == NULL) {
        return TA_FALSE;
    }

    // We need to find the sequence which we do by simply iterating over each one and comparing it's name.
    for (taUInt32 iSequence = 0; iSequence < pGAF->sequenceCount; ++iSequence) {
        // The sequence pointers are located at byte position 12.
        if (!taSeekFile(pGAF->pFile, 12 + (iSequence * sizeof(taUInt32)), taSeekOriginStart)) {
            return TA_FALSE;
        }

        taUInt32 sequencePointer;
        if (!taReadFileUInt32(pGAF->pFile, &sequencePointer)) {
            return TA_FALSE;
        }

        if (!taSeekFile(pGAF->pFile, sequencePointer, taSeekOriginStart)) {
            return TA_FALSE;
        }


        taUInt16 frameCount;
        if (!taReadFileUInt16(pGAF->pFile, &frameCount)) {
            return TA_FALSE;
        }

        if (!taSeekFile(pGAF->pFile, 6, taSeekOriginCurrent)) {
            return TA_FALSE;
        }

        // The file will be sitting on the sequence's name, so we just need to compare. If they're not equal just try the next sequence.
        const char* pName = pGAF->pFile->pFileData + taTellFile(pGAF->pFile);
        if (_stricmp(pName, sequenceName) == 0) {
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

taBool32 taGAFSelectSequenceByIndex(taGAF* pGAF, taUInt32 index, taUInt32* pFrameCountOut)
{
    if (pFrameCountOut) {
        *pFrameCountOut = 0;
    }

    if (pGAF == NULL || index >= pGAF->sequenceCount) {
        return TA_FALSE;
    }

    // The sequence pointers are located at byte position 12.
    if (!taSeekFile(pGAF->pFile, 12 + (index * sizeof(taUInt32)), taSeekOriginStart)) {
        return TA_FALSE;
    }

    taUInt32 sequencePointer;
    if (!taReadFileUInt32(pGAF->pFile, &sequencePointer)) {
        return TA_FALSE;
    }

    if (!taSeekFile(pGAF->pFile, sequencePointer, taSeekOriginStart)) {
        return TA_FALSE;
    }


    taUInt16 frameCount;
    if (!taReadFileUInt16(pGAF->pFile, &frameCount)) {
        return TA_FALSE;
    }

    if (!taSeekFile(pGAF->pFile, 6, taSeekOriginCurrent)) {
        return TA_FALSE;
    }

    pGAF->_sequenceName = pGAF->pFile->pFileData + taTellFile(pGAF->pFile);
    pGAF->_sequencePointer = sequencePointer;
    pGAF->_sequenceFrameCount = frameCount;

    if (pFrameCountOut) {
        *pFrameCountOut = frameCount;
    }

    return TA_TRUE;
}

taResult taGAFGetFrame(taGAF* pGAF, taUInt32 frameIndex, taUInt16* pWidthOut, taUInt16* pHeightOut, taInt16* pPosXOut, taInt16* pPosYOut, taUInt8** ppImageData)
{
    if (ppImageData) {
        *ppImageData = NULL;
    }

    if (pGAF == NULL || frameIndex >= pGAF->_sequenceFrameCount || pWidthOut == NULL || pHeightOut == NULL || pPosXOut == NULL || pPosYOut == NULL) {
        return TA_ERROR;
    }

    // Must have an sequence selected.
    if (pGAF->_sequencePointer == 0) {
        return TA_ERROR;
    }


    // Each frame pointer is grouped as a 64-bit value. We want the first 32-bits. The frame pointers start 40 bytes
    // after the beginning of the sequence.
    if (!taSeekFile(pGAF->pFile, pGAF->_sequencePointer + 40 + (frameIndex * sizeof(taUInt64)), taSeekOriginStart)) {
        return TA_ERROR;
    }

    taUInt32 framePointer;
    if (!taReadFileUInt32(pGAF->pFile, &framePointer)) {
        return TA_ERROR;
    }

    if (!taSeekFile(pGAF->pFile, framePointer, taSeekOriginStart)) {
        return TA_ERROR;
    }

    taGAFFrameHeader frameHeader;
    if (!taGAFReadFrameHeader(pGAF, &frameHeader)) {
        return TA_ERROR;
    }

    *pWidthOut = frameHeader.width;
    *pHeightOut = frameHeader.height;
    *pPosXOut = frameHeader.offsetX;
    *pPosYOut = frameHeader.offsetY;
    
    if (ppImageData != NULL) {
        taUInt8* pImageData = malloc(frameHeader.width * frameHeader.height);
        if (pImageData == NULL) {
            return TA_ERROR;
        }

        // It's important to clear the image data to the transparent color due to the way we'll be building the frame.
        memset(pImageData, TA_TRANSPARENT_COLOR, frameHeader.width * frameHeader.height);

        // We need to branch depending on whether or not we are loading pixel data or sub-frames.
        if (frameHeader.subframeCount == 0) {
            // It's raw pixel data.
            if (!taGAFReadFramePixels(pGAF, &frameHeader, frameHeader.width, frameHeader.height, frameHeader.offsetX, frameHeader.offsetY, pImageData)) {
                free(pImageData);
                return TA_ERROR;
            }
        } else {
            // The frame is made up of a bunch of sub-frames. They need to be combined by simply layering them on top
            // of each other.
            for (unsigned short iSubframe = 0; iSubframe < frameHeader.subframeCount; ++iSubframe) {
                // frameHeader.dataPtr points to a list of frameHeader.subframeCount pointers to frame headers.
                if (!taSeekFile(pGAF->pFile, frameHeader.dataPtr + (iSubframe * 4), taSeekOriginStart)) {
                    free(pImageData);
                    return TA_ERROR;
                }

                taUInt32 subframePointer;
                if (!taReadFileUInt32(pGAF->pFile, &subframePointer)) {
                    free(pImageData);
                    return TA_FALSE;
                }

                if (!taSeekFile(pGAF->pFile, subframePointer, taSeekOriginStart)) {
                    return TA_ERROR;
                }

                taGAFFrameHeader subframeHeader;
                if (!taGAFReadFrameHeader(pGAF, &subframeHeader)) {
                    return TA_ERROR;
                }

                if (!taGAFReadFramePixels(pGAF, &subframeHeader, frameHeader.width, frameHeader.height, frameHeader.offsetX, frameHeader.offsetY, pImageData)) {
                    free(pImageData);
                    return TA_ERROR;
                }
            }
        }

        if (ppImageData) {
            *ppImageData = pImageData;
        }
    }

    return TA_SUCCESS;
}

const char* taGAFGetCurrentSequenceName(taGAF* pGAF)
{
    if (pGAF == NULL) {
        return NULL;
    }

    return pGAF->_sequenceName;
}

void taGAFFree(void* pBuffer)
{
    free(pBuffer);
}



// GAF Texture Groups
// ==================
TA_INLINE char* taGAFTextureGroupCopySequenceName(char** ppNextStr, const char* src)
{
    char* pNextStr = *ppNextStr;
    size_t srcLen = strlen(src);
    memcpy(pNextStr, src, srcLen+1);

    *ppNextStr += srcLen+1;
    return pNextStr;
}

TA_PRIVATE taResult taGAFTextureGroupCreateTextureAtlas(taEngineContext* pEngine, taGAFTextureGroup* pGroup, taTexturePacker* pPacker, taColorMode colorMode, taTexture** ppTexture)
{
    assert(pEngine != NULL);
    assert(pGroup != NULL);
    assert(pPacker != NULL);
    assert(ppTexture != NULL);

    if (colorMode == taColorModePalette) {
        *ppTexture = taCreateTexture(pEngine->pGraphics, pPacker->width, pPacker->height, 1, pPacker->pImageData);
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

        *ppTexture = taCreateTexture(pEngine->pGraphics, pPacker->width, pPacker->height, 4, pImageData);
        if (*ppTexture == NULL) {
            free(pImageData);
            return TA_FAILED_TO_CREATE_RESOURCE;
        }

        free(pImageData);
    }

    return TA_SUCCESS;
}

taResult taGAFTextureGroupInit(taEngineContext* pEngine, const char* filePath, taColorMode colorMode, taGAFTextureGroup* pGroup)
{
    if (pGroup == NULL) {
        return TA_INVALID_ARGS;
    }

    taZeroObject(pGroup);
    
    pGroup->pEngine = pEngine;

    taGAF* pGAF = taOpenGAF(pEngine->pFS, filePath);
    if (pGAF == NULL) {
        return TA_FILE_NOT_FOUND;
    }


    // We load in two passes. The first pass is used to calculate the size of the memory allocation and the second
    // pass performs the actual loading.

    taTexturePacker packer;
    taTexturePackerInit(&packer, TA_MAX_TEXTURE_ATLAS_SIZE, TA_MAX_TEXTURE_ATLAS_SIZE, 1, TA_TEXTURE_PACKER_FLAG_HARD_EDGE);

    // PASS #1
    // =======
    taUInt32 totalSequenceCount = 0;
    taUInt32 totalFrameCount = 0;
    taUInt32 totalAtlasCount = 0;

    size_t payloadSize = 0;
    for (taUInt32 iSequence = 0; iSequence < pGAF->sequenceCount; ++iSequence) {
        payloadSize += sizeof(taGAFTextureGroupSequence);
        totalSequenceCount += 1;

        taUInt32 frameCount;
        if (taGAFSelectSequenceByIndex(pGAF, iSequence, &frameCount)) {
            payloadSize += strlen(taGAFGetCurrentSequenceName(pGAF))+1;

            for (taUInt32 iFrame = 0; iFrame < frameCount; ++iFrame) {
                payloadSize += sizeof(taGAFTextureGroupFrame);
                totalFrameCount += 1;

                taUInt16 sizeX;
                taUInt16 sizeY;
                taInt16 posX;
                taInt16 posY;
                if (taGAFGetFrame(pGAF, iFrame, &sizeX, &sizeY, &posX, &posY, NULL) == TA_SUCCESS) {
                    if (!taTexturePackerPackSubTexture(&packer, sizeX, sizeY, NULL, NULL)) {
                        // We failed to pack the subtexture which probably means there's not enough room. We just need to reset this packer and try again.
                        totalAtlasCount += 1;
                        taTexturePackerReset(&packer);
                        taTexturePackerPackSubTexture(&packer, sizeX, sizeY, NULL, NULL);   // <-- It doesn't really matter if this fails - we'll handle it organically later on in the second pass.
                    }
                }
            }
        }
    }

    // There might be textures still sitting in the packer needing to be uploaded.
    if (!taTexturePackerIsEmpty(&packer)) {
        totalAtlasCount += 1;
    }

    payloadSize += sizeof(taTexture*) * totalAtlasCount;


    size_t atlasesPayloadOffset       = 0;
    size_t sequencesPayloadOffset     = atlasesPayloadOffset   + (sizeof(taTexture*)                   * totalAtlasCount);
    size_t framesPayloadOffset        = sequencesPayloadOffset + (sizeof(taGAFTextureGroupSequence) * totalSequenceCount);
    size_t sequenceNamesPayloadOffset = framesPayloadOffset    + (sizeof(taGAFTextureGroupFrame)    * totalFrameCount);

    pGroup->_pPayload = (taUInt8*)calloc(1, payloadSize);
    if (pGroup->_pPayload == NULL) {
        taCloseGAF(pGAF);
        return TA_OUT_OF_MEMORY;
    }

    pGroup->ppAtlases  = (taTexture**                  )(pGroup->_pPayload + atlasesPayloadOffset);
    pGroup->pSequences = (taGAFTextureGroupSequence*)(pGroup->_pPayload + sequencesPayloadOffset);
    pGroup->pFrames    = (taGAFTextureGroupFrame*   )(pGroup->_pPayload + framesPayloadOffset);

    pGroup->atlasCount    = totalAtlasCount;
    pGroup->sequenceCount = totalSequenceCount;
    pGroup->frameCount    = totalFrameCount;


    // PASS #2
    // =======
    char* pNextStr = (char*)(pGroup->_pPayload + sequenceNamesPayloadOffset);
    taTexturePackerReset(&packer);

    totalSequenceCount = 0;
    totalFrameCount    = 0;
    totalAtlasCount    = 0;

    for (taUInt32 iSequence = 0; iSequence < pGAF->sequenceCount; ++iSequence) {
        taUInt32 frameCount;
        if (taGAFSelectSequenceByIndex(pGAF, iSequence, &frameCount)) {
            pGroup->pSequences[totalSequenceCount].name            = taGAFTextureGroupCopySequenceName(&pNextStr, taGAFGetCurrentSequenceName(pGAF));
            pGroup->pSequences[totalSequenceCount].firstFrameIndex = totalFrameCount;
            pGroup->pSequences[totalSequenceCount].frameCount      = frameCount;

            totalSequenceCount += 1;
            for (taUInt32 iFrame = 0; iFrame < frameCount; ++iFrame) {
                taUInt16 sizeX;
                taUInt16 sizeY;
                taInt16 posX;
                taInt16 posY;
                taUInt8* pImageData;
                if (taGAFGetFrame(pGAF, iFrame, &sizeX, &sizeY, &posX, &posY, &pImageData) == TA_SUCCESS) {
                    taTexturePackerSlot slot;
                    if (!taTexturePackerPackSubTexture(&packer, sizeX, sizeY, pImageData, &slot)) {
                        // We failed to pack the subtexture which probably means there's not enough room. We just need to reset this packer and try again.
                        taGAFTextureGroupCreateTextureAtlas(pEngine, pGroup, &packer, colorMode, &pGroup->ppAtlases[totalAtlasCount]);
                        totalAtlasCount += 1;

                        taTexturePackerReset(&packer);
                        taTexturePackerPackSubTexture(&packer, sizeX, sizeY, pImageData, &slot);
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
    if (!taTexturePackerIsEmpty(&packer)) {
        taGAFTextureGroupCreateTextureAtlas(pEngine, pGroup, &packer, colorMode, &pGroup->ppAtlases[totalAtlasCount]);
        totalAtlasCount += 1;
    }


    return TA_SUCCESS;
}

taResult taGAFTextureGroupUninit(taGAFTextureGroup* pGroup)
{
    if (pGroup == NULL) {
        return TA_INVALID_ARGS;
    }

    free(pGroup->_pPayload);
    return TA_SUCCESS;
}

taBool32 taGAFTextureGroupFindSequenceByName(taGAFTextureGroup* pGroup, const char* sequenceName, taUInt32* pSequenceIndex)
{
    if (pSequenceIndex) {
        *pSequenceIndex = (taUInt32)-1;
    }

    if (pGroup == NULL || sequenceName == NULL) {
        return TA_FALSE;
    }

    for (taUInt32 iSequence = 0; iSequence < pGroup->sequenceCount; ++iSequence) {
        if (_stricmp(pGroup->pSequences[iSequence].name, sequenceName) == 0) {
            if (*pSequenceIndex) *pSequenceIndex = iSequence;
            return TA_TRUE;
        }
    }

    return TA_FALSE;
}