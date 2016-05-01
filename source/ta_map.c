// Public domain. See "unlicense" statement at the end of this file.

// TODO:
// - Cull invisible terrain chunks.

// The maximum size of the texture atlas. We don't really want to use the GPU's maximum texture size
// because it can result in excessive wastage - modern GPUs support 16K textures, which is much more
// than we need and it's better to not needlessly waste the player's system resources. Keep this at
// a power of 2.
#define TA_MAX_TEXTURE_ATLAS_SIZE   512 /*4096*/

typedef struct
{
    uint32_t id;
    uint32_t width;
    uint32_t height;
    uint32_t mapdataPtr;
    uint32_t mapattrPtr;
    uint32_t tilegfxPtr;
    uint32_t tileCount;
    uint32_t featureTypesCount;
    uint32_t featureTypesPtr;
    uint32_t seaLevel;
    uint32_t minimapPtr;
} ta_tnt_header;

typedef struct
{
    uint32_t posX;
    uint32_t posY;
    uint32_t textureIndex;
} ta_tnt_tile_subimage;


bool ta_map__create_and_push_texture(ta_map_instance* pMap, ta_texture_packer* pPacker)
{
    ta_texture* pNewTexture = ta_create_texture(pMap->pGame->pGraphics, pPacker->width, pPacker->height, 1, pPacker->pImageData);
    if (pNewTexture == NULL) {
        return false;
    }

    ta_texture** ppNewTextures = realloc(pMap->ppTextures, (pMap->textureCount + 1) * sizeof(*pMap->ppTextures));
    if (ppNewTextures == NULL) {
        ta_delete_texture(pNewTexture);
        return false;
    }

    pMap->ppTextures = ppNewTextures;
    pMap->ppTextures[pMap->textureCount++] = pNewTexture;

    ta_texture_packer_reset(pPacker);
    return true;
}

bool ta_map__pack_subtexture(ta_map_instance* pMap, ta_texture_packer* pPacker, uint32_t width, uint32_t height, const void* pImageData, ta_texture_packer_slot* pSlotOut)
{
    // If we can't pack the image we just create a new texture on the graphics system and then reset the packer and try again.
    if (ta_texture_packer_pack_subtexture(pPacker, width, height, pImageData, pSlotOut)) {
        return true;
    }

    if (!ta_map__create_and_push_texture(pMap, pPacker)) {  // <-- This will reset the texture packer.
        return false;
    }

    return ta_texture_packer_pack_subtexture(pPacker, width, height, pImageData, pSlotOut);
}

int ta_map__sort_feature_types_by_filename(const void* a, const void* b)
{
    const ta_map_feature_type* pFeatureTypeA = a;
    const ta_map_feature_type* pFeatureTypeB = b;

    if (pFeatureTypeA->pDesc == NULL || pFeatureTypeB->pDesc == NULL) {
        return -1;
    }

    return strcmp(pFeatureTypeA->pDesc->filename, pFeatureTypeB->pDesc->filename);
}

int ta_map__sort_feature_types_by_index(const void* a, const void* b)
{
    const ta_map_feature_type* pFeatureTypeA = a;
    const ta_map_feature_type* pFeatureTypeB = b;

    if (pFeatureTypeA->_index <  pFeatureTypeB->_index) return -1;
    if (pFeatureTypeA->_index >  pFeatureTypeB->_index) return +1;

    return 0;
}

ta_map_feature_sequence* ta_map__load_gaf_sequence(ta_map_instance* pMap, ta_texture_packer* pPacker, ta_gaf* pGAF, const char* sequenceName)
{
    uint32_t frameCount;
    if (!ta_gaf_select_entry(pGAF, sequenceName, &frameCount)) {
        return NULL;
    }

    if (frameCount == 0) {
        return NULL;
    }

    ta_map_feature_sequence* pSeq = malloc(sizeof(*pSeq) - sizeof(pSeq->pFrames) + (frameCount * sizeof(ta_map_feature_frame)));
    if (pSeq == NULL) {
        return NULL;
    }

    pSeq->frameCount = frameCount;

    for (uint32_t iFrame = 0; iFrame < frameCount; ++iFrame)
    {
        uint32_t frameWidth;
        uint32_t frameHeight;
        int32_t offsetX;
        int32_t offsetY;
        uint8_t* pFrameImageData = ta_gaf_get_frame(pGAF, iFrame, &frameWidth, &frameHeight, &offsetX, &offsetY);
        if (pFrameImageData == NULL) {
            free(pSeq);
            return NULL;
        }

        ta_texture_packer_slot slot;
        if (!ta_map__pack_subtexture(pMap, pPacker, frameWidth, frameHeight, pFrameImageData, &slot)) {
            free(pSeq);
            return NULL;
        }

        pSeq->pFrames[iFrame].width = frameWidth;
        pSeq->pFrames[iFrame].height = frameHeight;
        pSeq->pFrames[iFrame].offsetX = offsetX;
        pSeq->pFrames[iFrame].offsetY = offsetY;
        pSeq->pFrames[iFrame].texturePosX = slot.posX;
        pSeq->pFrames[iFrame].texturePosY = slot.posY;
        pSeq->pFrames[iFrame].textureIndex = pMap->textureCount;
    }

    return pSeq;
}

void ta_map__calculate_object_position_xy(uint32_t tileX, uint32_t tileY, uint16_t objectFootprintX, uint16_t objectFootprintY, float* pPosXOut, float* pPosYOut)
{
    assert(pPosXOut != NULL);
    assert(pPosYOut != NULL);

    float tileCenterX = (tileX * 16.0f) + 8.0f;
    float tileCenterY = (tileY * 16.0f) + 8.0f;

    *pPosXOut = tileCenterX + (objectFootprintX/2 * 16);
    *pPosYOut = tileCenterY + (objectFootprintX/2 * 16);
}


ta_file* ta_map__open_tnt_file(ta_fs* pFS, const char* mapName)
{
    char filename[TA_MAX_PATH];
    if (!drpath_copy_and_append(filename, sizeof(filename), "maps", mapName)) {
        return NULL;
    }
    if (!drpath_append_extension(filename, sizeof(filename), "tnt")) {
        return NULL;
    }

    return ta_open_file(pFS, filename, 0);
}

void ta_map__close_tnt_file(ta_file* pTNT)
{
    ta_close_file(pTNT);
}

bool ta_map__read_tnt_header(ta_file* pTNT, ta_tnt_header* pHeader)
{
    assert(pTNT != NULL);
    assert(pHeader != NULL);

    if (!ta_read_file_uint32(pTNT, &pHeader->id)) {
        return false;
    }

    if (pHeader->id != 8192) {
        return false;   // Not a TNT file.
    }


    if (!ta_read_file_uint32(pTNT, &pHeader->width) ||
        !ta_read_file_uint32(pTNT, &pHeader->height) ||
        !ta_read_file_uint32(pTNT, &pHeader->mapdataPtr) ||
        !ta_read_file_uint32(pTNT, &pHeader->mapattrPtr) ||
        !ta_read_file_uint32(pTNT, &pHeader->tilegfxPtr) ||
        !ta_read_file_uint32(pTNT, &pHeader->tileCount) ||
        !ta_read_file_uint32(pTNT, &pHeader->featureTypesCount) ||
        !ta_read_file_uint32(pTNT, &pHeader->featureTypesPtr) ||
        !ta_read_file_uint32(pTNT, &pHeader->seaLevel) ||
        !ta_read_file_uint32(pTNT, &pHeader->minimapPtr) ||
        !ta_seek_file(pTNT, 20, ta_seek_origin_current))    // <-- Last 20 bytes are unused.
    {
        return false;
    }

    return true;
}

bool ta_map__load_tnt(ta_map_instance* pMap, const char* mapName, ta_texture_packer* pPacker)
{
    assert(pMap != NULL);
    assert(mapName != NULL);
    assert(pPacker != NULL);

    ta_file* pTNT = ta_map__open_tnt_file(pMap->pGame->pFS, mapName);
    if (pTNT == NULL) {
        return false;
    }

    // Header.
    ta_tnt_header header;
    if (!ta_map__read_tnt_header(pTNT, &header)) {
        return false;
    }


    pMap->terrain.tileCountX = header.width/2;
    pMap->terrain.tileCountY = header.height/2;

    pMap->terrain.chunkCountX = pMap->terrain.tileCountX / TA_TERRAIN_CHUNK_SIZE;
    if ((pMap->terrain.chunkCountX % TA_TERRAIN_CHUNK_SIZE) > 0) {
        pMap->terrain.chunkCountX += 1;
    }

    pMap->terrain.chunkCountY = pMap->terrain.tileCountY / TA_TERRAIN_CHUNK_SIZE;
    if ((pMap->terrain.chunkCountY % TA_TERRAIN_CHUNK_SIZE) > 0) {
        pMap->terrain.chunkCountY += 1;
    }


    // At this point we have enough information to allocate memory for per-tile data.
    uint32_t tilesPerChunk = TA_TERRAIN_CHUNK_SIZE*TA_TERRAIN_CHUNK_SIZE;
    uint32_t totalChunkCount = pMap->terrain.chunkCountX * pMap->terrain.chunkCountY;
    uint32_t totalTileCount = tilesPerChunk * totalChunkCount;

    pMap->terrain.pChunks = calloc(totalChunkCount, sizeof(*pMap->terrain.pChunks));
    if (pMap->terrain.pChunks == NULL) {
        goto on_error;
    }

    ta_vertex_p2t2* pVertexData = malloc(totalTileCount*4 * sizeof(ta_vertex_p2t2));
    if (pVertexData == NULL) {
        goto on_error;
    }

    uint32_t* pIndexData = malloc(totalTileCount*4 * sizeof(uint32_t));
    if (pIndexData == NULL) {
        free(pVertexData);
        goto on_error;
    }


    // We need to pack the tile graphics into textures before we can generate the terrain meshes. We
    // use the packer to do this. For each tile we pack we need to track a little bit of information
    // so we can split the meshes properly.
    ta_tnt_tile_subimage* pTileSubImages = malloc(header.tileCount * sizeof(*pTileSubImages));
    if (pTileSubImages == NULL) {
        free(pIndexData);
        free(pVertexData);
        goto on_error;
    }

    // For every tile, pack it's graphic into a texture.
    if (!ta_seek_file(pTNT, header.tilegfxPtr, ta_seek_origin_start)) {
        free(pIndexData);
        free(pVertexData);
        free(pTileSubImages);
        goto on_error;
    }

    for (uint32_t iTile = 0; iTile < header.tileCount; /* DO NOTHING */)
    {
        char* pTileImageData = pTNT->pFileData + ta_tell_file(pTNT) + (iTile*32*32);

        ta_texture_packer_slot slot;
        if (ta_texture_packer_pack_subtexture(pPacker, 32, 32, pTileImageData, &slot))
        {
            pTileSubImages[iTile].posX = slot.posX;
            pTileSubImages[iTile].posY = slot.posY;
            pTileSubImages[iTile].textureIndex = pMap->textureCount;

            iTile += 1;
        }
        else
        {
            // We failed to pack the tile into the atlas. Likely we just ran out of room. Just commit that
            // texture and start a fresh one and try this tile again.
            if (!ta_map__create_and_push_texture(pMap, pPacker)) {  // <-- This will reset the texture packer.
                free(pIndexData);
                free(pVertexData);
                free(pTileSubImages);
                goto on_error;
            }
        }
    }


    
    // For every chunk...
    for (uint32_t chunkY = 0; chunkY < pMap->terrain.chunkCountY; ++chunkY)
    {
        uint32_t chunkTileCountY = TA_TERRAIN_CHUNK_SIZE;
        if ((chunkY*TA_TERRAIN_CHUNK_SIZE) + chunkTileCountY > pMap->terrain.tileCountY) {
            chunkTileCountY = pMap->terrain.tileCountY - (chunkY*TA_TERRAIN_CHUNK_SIZE);
        }

        for (uint32_t chunkX = 0; chunkX < pMap->terrain.chunkCountX; ++chunkX)
        {
            uint32_t chunkTileCountX = TA_TERRAIN_CHUNK_SIZE;
            if ((chunkX*TA_TERRAIN_CHUNK_SIZE) + chunkTileCountX > pMap->terrain.tileCountX) {
                chunkTileCountX = pMap->terrain.tileCountX - (chunkX*TA_TERRAIN_CHUNK_SIZE);
            }


            ta_map_terrain_chunk* pChunk = pMap->terrain.pChunks + ((chunkY*pMap->terrain.chunkCountX) + chunkX);

            uint32_t chunkVertexOffset = ((chunkY*pMap->terrain.chunkCountX) + chunkX) * (tilesPerChunk*4);
            uint32_t chunkIndexOffset  = ((chunkY*pMap->terrain.chunkCountX) + chunkX) * (tilesPerChunk*4);


            // We want to group each mesh in the chunk by texture...
            for (uint32_t iTexture = 0; iTexture < pMap->textureCount+1; ++iTexture)    // <-- +1 because there is a texture sitting in the packer that hasn't yet been added to the list.
            {
                bool isMeshAllocatedForThisTextures = false;
                
                // For every tile in the chunk...
                for (uint32_t tileY = 0; tileY < chunkTileCountY; ++tileY)
                {
                    // Seek to the first tile in this row.
                    uint32_t firstTileOnRow = ((chunkY*TA_TERRAIN_CHUNK_SIZE + tileY) * pMap->terrain.tileCountX) + (chunkX*TA_TERRAIN_CHUNK_SIZE);
                    if (!ta_seek_file(pTNT, header.mapdataPtr + (firstTileOnRow * sizeof(uint16_t)), ta_seek_origin_start)) {
                        free(pIndexData);
                        free(pVertexData);
                        free(pTileSubImages);
                        goto on_error;
                    }

                    for (uint32_t tileX = 0; tileX < chunkTileCountX; ++tileX)
                    {
                        uint16_t tileIndex;
                        if (!ta_read_file_uint16(pTNT, &tileIndex)) {
                            free(pIndexData);
                            free(pVertexData);
                            free(pTileSubImages);
                            goto on_error;
                        }

                        if (pTileSubImages[tileIndex].textureIndex == iTexture)
                        {
                            // The tile is using this texture. Ensure there is a mesh allocated for it...
                            if (!isMeshAllocatedForThisTextures)
                            {
                                // This is the first vertex for this texture so we'll need to allocate a mesh.
                                ta_map_terrain_submesh* pNewMeshes = realloc(pChunk->pMeshes, (pChunk->meshCount+1) * sizeof(*pChunk->pMeshes));
                                if (pNewMeshes == NULL) {
                                    free(pIndexData);
                                    free(pVertexData);
                                    free(pTileSubImages);
                                    goto on_error;
                                }

                                pNewMeshes[pChunk->meshCount].textureIndex = iTexture;
                                pNewMeshes[pChunk->meshCount].indexCount = 0;
                                pNewMeshes[pChunk->meshCount].indexOffset = chunkIndexOffset;

                                pChunk->pMeshes = pNewMeshes;
                                pChunk->meshCount += 1;

                                isMeshAllocatedForThisTextures = true;
                            }
                            
                            // We always add the vertex to the most recent mesh.
                            ta_map_terrain_submesh* pMesh = pChunk->pMeshes + (pChunk->meshCount - 1);
                            
                            ta_vertex_p2t2* pQuad = pVertexData + chunkVertexOffset;

                            float tileU = pTileSubImages[tileIndex].posX / (float)pPacker->width;
                            float tileV = pTileSubImages[tileIndex].posY / (float)pPacker->height;
                            
                            // Top left.
                            pQuad[0].x = (float)(chunkX*TA_TERRAIN_CHUNK_SIZE + tileX) * 32.0f;
                            pQuad[0].y = (float)(chunkY*TA_TERRAIN_CHUNK_SIZE + tileY) * 32.0f;
                            pQuad[0].u = tileU;
                            pQuad[0].v = tileV;

                            // Bottom left
                            pQuad[1].x = pQuad[0].x;
                            pQuad[1].y = pQuad[0].y + 32;
                            pQuad[1].u = pQuad[0].u;
                            pQuad[1].v = pQuad[0].v + (32.0f / pPacker->height);

                            // Bottom right
                            pQuad[2].x = pQuad[1].x + 32;
                            pQuad[2].y = pQuad[1].y;
                            pQuad[2].u = pQuad[1].u + (32.0f / pPacker->width);
                            pQuad[2].v = pQuad[1].v;

                            // Top right
                            pQuad[3].x = pQuad[2].x;
                            pQuad[3].y = pQuad[2].y - 32;
                            pQuad[3].u = pQuad[2].u;
                            pQuad[3].v = pQuad[2].v - (32.0f / pPacker->height);



                            uint32_t* pQuadIndices = pIndexData + chunkIndexOffset;
                            pQuadIndices[0] = chunkVertexOffset + 0;
                            pQuadIndices[1] = chunkVertexOffset + 1;
                            pQuadIndices[2] = chunkVertexOffset + 2;
                            pQuadIndices[3] = chunkVertexOffset + 3;
                            pMesh->indexCount += 4;


                            chunkVertexOffset += 4;
                            chunkIndexOffset += 4;
                        }
                    }
                }
            }
        }
    }

    free(pTileSubImages);

    // Finally we can create the terrains mesh.
    pMap->terrain.pMesh = ta_create_mesh(pMap->pGame->pGraphics, ta_primitive_type_quad, ta_vertex_format_p2t2, totalTileCount*4, pVertexData, ta_index_format_uint32, totalTileCount*4, pIndexData);
    if (pMap->terrain.pMesh == NULL) {
        free(pIndexData);
        free(pVertexData);
        goto on_error;
    }

    free(pIndexData);
    free(pVertexData);



    // At this point the graphics for the terrain will have been loaded, so now we need to move on to the features. The feature
    // types are listed in a group and we use these names to find the descriptors of each one. Once we have the descriptors we
    // simply sort them by file name and load each one.
    if (!ta_seek_file(pTNT, header.featureTypesPtr, ta_seek_origin_start)) {
        goto on_error;
    }

    pMap->featureTypesCount = header.featureTypesCount;
    pMap->pFeatureTypes = calloc(header.featureTypesCount, sizeof(*pMap->pFeatureTypes));
    if (pMap->pFeatureTypes == NULL) {
        goto on_error;
    }

    for (uint32_t iFeatureType = 0; iFeatureType < pMap->featureTypesCount; ++iFeatureType) {
        if (!ta_seek_file(pTNT, 4, ta_seek_origin_current)) {
            goto on_error;
        }

        if (!ta_read_file(pTNT, pMap->pFeatureTypes[iFeatureType].name, 128, NULL)) {
            goto on_error;
        }

        pMap->pFeatureTypes[iFeatureType].pDesc = ta_find_feature_desc(pMap->pGame->pFeatures, pMap->pFeatureTypes[iFeatureType].name);
        pMap->pFeatureTypes[iFeatureType]._index = iFeatureType;
        //printf("%s\n", pMap->pFeatureTypes[iFeatureType].pDesc->filename);
    }

    // The feature types need to be sorted by their file name so we can load them efficiently.
    qsort(pMap->pFeatureTypes, pMap->featureTypesCount, sizeof(*pMap->pFeatureTypes), ta_map__sort_feature_types_by_filename);


    ta_gaf* pCurrentGAF = NULL;
    for (uint32_t iFeatureType = 0; iFeatureType < pMap->featureTypesCount; ++iFeatureType)
    {
        ta_map_feature_type* pFeatureType = &pMap->pFeatureTypes[iFeatureType];
        if (pFeatureType->pDesc->filename[0] != '\0')
        {
            if (pCurrentGAF == NULL || _stricmp(pCurrentGAF->filename, pFeatureType->pDesc->filename) != 0)
            {
                // A new GAF file needs to be loaded.
                ta_close_gaf(pCurrentGAF);
                pCurrentGAF = ta_open_gaf(pMap->pGame->pFS, pFeatureType->pDesc->filename);
                if (pCurrentGAF == NULL) {
                    goto on_error;
                }
            }

            // At this point the GAF file containing the feature should be loaded and we just need to read it's frame data for
            // every required sequence.
            pFeatureType->pSequenceDefault = ta_map__load_gaf_sequence(pMap, pPacker, pCurrentGAF, pFeatureType->pDesc->seqname);
            pFeatureType->pSequenceBurn = ta_map__load_gaf_sequence(pMap, pPacker, pCurrentGAF, pFeatureType->pDesc->seqnameburn);
            pFeatureType->pSequenceDie = ta_map__load_gaf_sequence(pMap, pPacker, pCurrentGAF, pFeatureType->pDesc->seqnamedie);
            pFeatureType->pSequenceReclamate = ta_map__load_gaf_sequence(pMap, pPacker, pCurrentGAF, pFeatureType->pDesc->seqnamereclamate);
            pFeatureType->pSequenceShadow = ta_map__load_gaf_sequence(pMap, pPacker, pCurrentGAF, pFeatureType->pDesc->seqnameshadow);
        }
        else
        {
            // It's not a 2D feature so assume it's a 3D one.
            ta_3do* p3DO = ta_open_3do(pMap->pGame->pFS, pFeatureType->pDesc->object);
            if (p3DO != NULL)
            {
                // TODO: Load 3DO file.
                ta_close_3do(p3DO);
            }
        }
    }

    ta_close_gaf(pCurrentGAF);



    // Features are loaded by iterating over each 16x16 tile. The type of each feature is determine based on an index, however
    // remember from earlier that we sorted the features which means those indexes are no longer valid. To address this we just
    // sort it back to it's original original order.
    qsort(pMap->pFeatureTypes, pMap->featureTypesCount, sizeof(*pMap->pFeatureTypes), ta_map__sort_feature_types_by_index);

    if (!ta_seek_file(pTNT, header.mapattrPtr, ta_seek_origin_start)) {
        goto on_error;
    }

    uint32_t featureCount = 0;
    for (uint32_t y = 0; y < header.height; ++y) {
        for (uint32_t x = 0; x < header.width; ++x) {
            uint32_t tile;
            if (!ta_read_file_uint32(pTNT, &tile)) {
                goto on_error;
            }

            uint16_t featureTypeIndex = (uint16_t)((tile & 0x00FFFF00) >> 8);
            if (featureTypeIndex != 0xFFFF && featureTypeIndex != 0xFFFC && featureTypeIndex != 0xFFFE) {
                featureCount += 1;
            }
        }
    }


    if (!ta_seek_file(pTNT, header.mapattrPtr, ta_seek_origin_start)) {
        goto on_error;
    }

    pMap->pFeatures = malloc(featureCount * sizeof(*pMap->pFeatures));
    if (pMap->pFeatures == NULL) {
        goto on_error;
    }

    pMap->featureCount = 0;
    for (uint32_t y = 0; y < header.height; ++y) {
        for (uint32_t x = 0; x < header.width; ++x) {
            uint32_t tile;
            if (!ta_read_file_uint32(pTNT, &tile)) {
                goto on_error;
            }

            uint8_t tileHeight = (uint8_t)((tile & 0xFF));

            uint16_t featureTypeIndex = (uint16_t)((tile & 0x00FFFF00) >> 8);
            if (featureTypeIndex != 0xFFFF && featureTypeIndex != 0xFFFC && featureTypeIndex != 0xFFFE)
            {
                ta_map_feature feature;
                feature.pType = &pMap->pFeatureTypes[featureTypeIndex];
                feature.pCurrentSequence = feature.pType->pSequenceDefault;
                feature.currentFrameIndex = 0;

                ta_map__calculate_object_position_xy(x, y, feature.pType->pDesc->footprintX, feature.pType->pDesc->footprintY, &feature.posX, &feature.posY);
                feature.posZ = tileHeight;  // <-- FIXME: This is not exactly correct. Need to calculate the height of the center point of the tile based on the surrounding tiles.

                // Add the feature to the list.
                pMap->pFeatures[pMap->featureCount++] = feature;
            }
        }
    }
    
    
    ta_map__close_tnt_file(pTNT);
    return true;


on_error:
    ta_map__close_tnt_file(pTNT);
    return false;
}


ta_config_obj* ta_map__open_ota_file(ta_fs* pFS, const char* mapName)
{
    char filename[TA_MAX_PATH];
    if (!drpath_copy_and_append(filename, sizeof(filename), "maps", mapName)) {
        return NULL;
    }
    if (!drpath_append_extension(filename, sizeof(filename), "ota")) {
        return NULL;
    }

    return ta_parse_config_from_file(pFS, filename);
}

void ta_map__close_ota_file(ta_config_obj* pOTA)
{
    ta_delete_config(pOTA);
}

bool ta_map__load_ota(ta_map_instance* pMap, const char* mapName)
{
    ta_config_obj* pOTA = ta_map__open_ota_file(pMap->pGame->pFS, mapName);
    if (pOTA == NULL) {
        return false;
    }

    // TODO: Do something.

    ta_map__close_ota_file(pOTA);
    return true;
}


ta_map_instance* ta_load_map(ta_game* pGame, const char* mapName)
{
    if (pGame == NULL || mapName == NULL) {
        return NULL;
    }

    // Clamp the texture size to avoid excessive wastage. Modern GPUs support 16K textures which is way more than we need, and
    // I'd rather avoid wasting the player's system resources.
    uint32_t maxTextureSize = ta_get_max_texture_size(pGame->pGraphics);
    if (maxTextureSize > TA_MAX_TEXTURE_ATLAS_SIZE) {
        maxTextureSize = TA_MAX_TEXTURE_ATLAS_SIZE;
    }

    // We'll need a texture packer to help us pack images into atlases.
    ta_texture_packer packer;
    if (!ta_texture_packer_init(&packer, maxTextureSize, maxTextureSize, 1)) {
        return NULL;
    }


    ta_map_instance* pMap = calloc(1, sizeof(*pMap));
    if (pMap == NULL) {
        ta_texture_packer_uninit(&packer);
        return NULL;
    }

    pMap->pGame = pGame;

    if (!ta_map__load_tnt(pMap, mapName, &packer)) {
        goto on_error;
    }

    if (!ta_map__load_ota(pMap, mapName)) {
        goto on_error;
    }

    
    // At the end of loading everything there could be a texture still sitting in the packer which needs to be created.
    if (packer.cursorPosX != 0 || packer.cursorPosY != 0) {
        if (!ta_map__create_and_push_texture(pMap, &packer)) {  // <-- This will reset the texture packer.
            goto on_error;
        }
    }
    

    ta_texture_packer_uninit(&packer);
    return pMap;


on_error:
    ta_texture_packer_uninit(&packer);
    ta_unload_map(pMap);
    return NULL;
}

void ta_unload_map(ta_map_instance* pMap)
{
    if (pMap == NULL) {
        return;
    }

    free(pMap->pFeatures);
    free(pMap->pFeatureTypes);
    ta_delete_mesh(pMap->terrain.pMesh);
    free(pMap->terrain.pChunks);
    free(pMap);
}