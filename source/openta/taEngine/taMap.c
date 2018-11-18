// Copyright (C) 2018 David Reid. See included LICENSE file.

// TODO:
// - Cull invisible terrain chunks.

typedef struct
{
    taUInt32 id;
    taUInt32 width;
    taUInt32 height;
    taUInt32 mapdataPtr;
    taUInt32 mapattrPtr;
    taUInt32 tilegfxPtr;
    taUInt32 tileCount;
    taUInt32 featureTypesCount;
    taUInt32 featureTypesPtr;
    taUInt32 seaLevel;
    taUInt32 minimapPtr;
} ta_tnt_header;

typedef struct
{
    taUInt32 posX;
    taUInt32 posY;
    taUInt32 textureIndex;
} ta_tnt_tile_subimage;

typedef struct
{
    char name[256];
    taUInt32 posX;
    taUInt32 posY;
    taUInt32 sizeX;
    taUInt32 sizeY;
    taUInt32 textureIndex;
} ta_map_loaded_texture;

typedef struct
{
    ta_texture_packer texturePacker;
    ta_texture_packer_slot paletteTextureSlot;

    size_t loadedTexturesBufferSize;
    size_t loadedTexturesCount;
    ta_map_loaded_texture* pLoadedTextures;

    size_t meshBuildersBufferSize;
    size_t meshBuildersCount;
    ta_mesh_builder* pMeshBuilders;
} ta_map_load_context;

taBool32 ta_map__create_and_push_texture(ta_map_instance* pMap, ta_texture_packer* pPacker)
{
    taTexture* pNewTexture = ta_create_texture(pMap->pEngine->pGraphics, pPacker->width, pPacker->height, 1, pPacker->pImageData);
    if (pNewTexture == NULL) {
        return TA_FALSE;
    }

    taTexture** ppNewTextures = realloc(pMap->ppTextures, (pMap->textureCount + 1) * sizeof(*pMap->ppTextures));
    if (ppNewTextures == NULL) {
        ta_delete_texture(pNewTexture);
        return TA_FALSE;
    }

    pMap->ppTextures = ppNewTextures;
    pMap->ppTextures[pMap->textureCount++] = pNewTexture;

    ta_texture_packer_reset(pPacker);
    return TA_TRUE;
}

taBool32 ta_map__pack_subtexture(ta_map_instance* pMap, ta_texture_packer* pPacker, taUInt32 width, taUInt32 height, const void* pImageData, ta_texture_packer_slot* pSlotOut)
{
    // If we can't pack the image we just create a new texture on the graphics system and then reset the packer and try again.
    if (ta_texture_packer_pack_subtexture(pPacker, width, height, pImageData, pSlotOut)) {
        return TA_TRUE;
    }

    if (!ta_map__create_and_push_texture(pMap, pPacker)) {  // <-- This will reset the texture packer.
        return TA_FALSE;
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

void ta_map__reset_mesh_builders(ta_map_load_context* pLoadContext)
{
    for (size_t i = 0; i < pLoadContext->meshBuildersCount; ++i) {
        ta_mesh_builder_reset(&pLoadContext->pMeshBuilders[i]);
    }

    pLoadContext->meshBuildersCount = 0;
}

ta_map_feature_sequence* ta_map__load_gaf_sequence(ta_map_instance* pMap, ta_texture_packer* pPacker, ta_gaf* pGAF, const char* sequenceName)
{
    taUInt32 frameCount;
    if (!ta_gaf_select_sequence(pGAF, sequenceName, &frameCount)) {
        return NULL;
    }

    if (frameCount == 0) {
        return NULL;
    }

    ta_map_feature_sequence* pSeq = malloc(sizeof(*pSeq) + (frameCount * sizeof(ta_map_feature_frame)));
    if (pSeq == NULL) {
        return NULL;
    }

    pSeq->frameCount = frameCount;

    for (taUInt32 iFrame = 0; iFrame < frameCount; ++iFrame)
    {
        taUInt32 frameWidth;
        taUInt32 frameHeight;
        taInt32 offsetX;
        taInt32 offsetY;
        taUInt8* pFrameImageData;
        if (ta_gaf_get_frame(pGAF, iFrame, &frameWidth, &frameHeight, &offsetX, &offsetY, &pFrameImageData) != TA_SUCCESS) {
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


taBool32 ta_map__load_texture(ta_map_instance* pMap, ta_map_load_context* pLoadContext, const char* textureName, ta_map_loaded_texture* pTextureOut)
{
    assert(pMap != NULL);
    assert(pLoadContext != NULL);
    assert(textureName != NULL);
    assert(pTextureOut != NULL);

    // If a texture of the same name has already been loaded, we just return that one.
    for (size_t i = 0; i < pLoadContext->loadedTexturesCount; ++i) {
        if (_stricmp(pLoadContext->pLoadedTextures[i].name, textureName) == 0) {
            *pTextureOut = pLoadContext->pLoadedTextures[i];
            return TA_TRUE;
        }
    }

    if (pLoadContext->loadedTexturesCount == pLoadContext->loadedTexturesBufferSize)
    {
        size_t newBufferSize = (pLoadContext->loadedTexturesBufferSize == 0) ? 16 : pLoadContext->loadedTexturesBufferSize*2;
        ta_map_loaded_texture* pNewTextures = (ta_map_loaded_texture*)realloc(pLoadContext->pLoadedTextures, newBufferSize * sizeof(*pNewTextures));
        if (pNewTextures == NULL) {
            return TA_FALSE;
        }

        pLoadContext->loadedTexturesBufferSize = newBufferSize;
        pLoadContext->pLoadedTextures = pNewTextures;
    }


    // Here is where the texture is loaded. To load the texture we need to find it from the list of always-open GAF files managed by
    // the game context.
    size_t i = pLoadContext->loadedTexturesCount;
    assert(i < pLoadContext->loadedTexturesBufferSize);

    for (size_t iGAF = 0; iGAF < pMap->pEngine->textureGAFCount; ++iGAF)
    {
        taUInt32 frameCount;
        if (ta_gaf_select_sequence(pMap->pEngine->ppTextureGAFs[iGAF], textureName, &frameCount))
        {
            taUInt32 width;
            taUInt32 height;
            taUInt32 posX;
            taUInt32 posY;
            taUInt8* pTextureData;
            if (ta_gaf_get_frame(pMap->pEngine->ppTextureGAFs[iGAF], 0, &width, &height, &posX, &posY, &pTextureData) != TA_SUCCESS) {
                return TA_FALSE;
            }

            // The texture was successfully loaded, so now it needs to be packed into an atlas.
            ta_texture_packer_slot subtextureSlot;
            if (!ta_map__pack_subtexture(pMap, &pLoadContext->texturePacker, width, height, pTextureData, &subtextureSlot)) {
                ta_gaf_free(pTextureData);
                return TA_FALSE;
            }

            ta_gaf_free(pTextureData);


            // The texture has been packed.
            strncpy_s(pLoadContext->pLoadedTextures[i].name, sizeof(pLoadContext->pLoadedTextures[i].name), textureName, _TRUNCATE);
            pLoadContext->pLoadedTextures[i].posX  = subtextureSlot.posX;
            pLoadContext->pLoadedTextures[i].posY  = subtextureSlot.posY;
            pLoadContext->pLoadedTextures[i].sizeX = subtextureSlot.width;
            pLoadContext->pLoadedTextures[i].sizeY = subtextureSlot.height;
            pLoadContext->pLoadedTextures[i].textureIndex = pMap->textureCount;

            *pTextureOut = pLoadContext->pLoadedTextures[i];
            return TA_TRUE;
        }
    }

    return TA_FALSE;
}

taUInt32 ta_map__load_3do_objects_recursive(ta_map_instance* pMap, ta_map_load_context* pLoadContext, taFile* pFile, ta_map_3do* p3DO, taUInt32 nextObjectIndex)
{
    assert(pMap != NULL);
    assert(pFile != NULL);
    assert(p3DO != NULL);
    assert(p3DO->objectCount > nextObjectIndex);

    // The file should be sitting on the first byte of the header of the object.
    ta3DOObjectHeader objectHeader;
    if (!ta3DOReadObjectHeader(pFile, &objectHeader)) {
        return 0;
    }

    taUInt32 thisIndex = nextObjectIndex;
    taUInt32 firstChildIndex = thisIndex + 1;

    p3DO->pObjects[thisIndex].meshCount = 0;
    p3DO->pObjects[thisIndex].relativePosX = objectHeader.relativePosX / 65536.0f;
    p3DO->pObjects[thisIndex].relativePosY = objectHeader.relativePosZ / 65536.0f;
    p3DO->pObjects[thisIndex].relativePosZ = objectHeader.relativePosY / 65536.0f;


    // Objects are made up of a bunch of primitives (points, lines, triangles and quads), with each individual primitive identifying
    // the texture to use with it. The texture needs to be loaded, but we need to ensure we don't load multiple copies of the same
    // texture. Unfortunately, there doesn't appear to be an easy way of pre-loading each texture before loading primitives, so texture
    // loading needs to be done per-primitive.
    //
    // When loading a new texture it may need to be placed in a different atlas to the others. In this case the object needs to be
    // split into multiple meshes.
    ta_map__reset_mesh_builders(pLoadContext);

    // Primitives.
    if (!taSeekFile(pFile, objectHeader.primitivePtr, taSeekOriginStart)) {
        return 0;
    }

    for (taUInt32 iPrim = 0; iPrim < objectHeader.primitiveCount; ++iPrim)
    {
        // The first thing to do is load the texture. We need to do this so we can know whether or not we should add the primitive to
        // an already-in-progress mesh or to start a new one. This is based on the index of the texture atlas the texture is contained
        // in.
        ta3DOPrimitiveHeader primHeader;
        if (!ta3DOReadPrimitiveHeader(pFile, &primHeader)) {
            return 0;
        }

        taBool32 isClear = TA_FALSE;
        taBool32 isColor = TA_FALSE;

        ta_map_loaded_texture texture;
        if (primHeader.textureNamePtr != 0)
        {
            const char* textureName = pFile->pFileData + primHeader.textureNamePtr;
            if (!ta_map__load_texture(pMap, pLoadContext, textureName, &texture)) {
                return 0;   // Failed to load the texture.
            }
        }
        else
        {
            // There is no texture. Need to handle this case, but not sure how... Draw it as a solid color using the color index? In this
            // case we could use a 1x1 texture that's located somewhere in the first 16x16 pixels of the first texture atlas.
            //
            // Don't forget about the isColored attribute. It's value seems inconsistent...
            isColor = primHeader.isColored != 0;
            if (!isColor) {
                isClear = TA_TRUE;
            }

            texture.textureIndex = 0;
            texture.posX = pLoadContext->paletteTextureSlot.posX;
            texture.posY = pLoadContext->paletteTextureSlot.posY;
            texture.sizeX = 1;
            texture.sizeY = 1;
        }


        if (!isClear) {
            // We need to find the mesh builder that's tied to the texture index. If it doesn't exist we'll need to create a new one.
            ta_mesh_builder* pMeshBuilder = NULL;
            for (size_t i = 0; i < pLoadContext->meshBuildersCount; ++i) {
                if (pLoadContext->pMeshBuilders[i].textureIndex == texture.textureIndex) {
                    pMeshBuilder = &pLoadContext->pMeshBuilders[i];
                    break;
                }
            }

            if (pMeshBuilder == NULL)
            {
                // There is no mesh builder associated with the texture index, so a fresh one will need to be created.
                if (pLoadContext->meshBuildersCount == pLoadContext->meshBuildersBufferSize)
                {
                    size_t newMeshBuildersBufferSize = pLoadContext->meshBuildersCount + 1;
                    ta_mesh_builder* pNewMeshBuilders = (ta_mesh_builder*)realloc(pLoadContext->pMeshBuilders, newMeshBuildersBufferSize * sizeof(*pNewMeshBuilders));
                    if (pNewMeshBuilders == NULL) {
                        return 0;
                    }

                    pLoadContext->meshBuildersBufferSize = newMeshBuildersBufferSize;
                    pLoadContext->pMeshBuilders = pNewMeshBuilders;

                    ta_mesh_builder_init(&pLoadContext->pMeshBuilders[pLoadContext->meshBuildersCount], sizeof(ta_vertex_p3t2n3));
                }

                size_t iMeshBuilder = pLoadContext->meshBuildersCount;
                assert(iMeshBuilder < pLoadContext->meshBuildersBufferSize);

                pLoadContext->pMeshBuilders[iMeshBuilder].textureIndex = texture.textureIndex;
                pLoadContext->meshBuildersCount += 1;

                pMeshBuilder = &pLoadContext->pMeshBuilders[iMeshBuilder];
            }

            assert(pMeshBuilder != NULL);

            if (primHeader.indexCount >= 3) {
                taUInt16* indices = (taUInt16*)(pFile->pFileData + primHeader.indexArrayPtr);

                float uvLeft;
                float uvBottom;
                float uvRight;
                float uvTop;
                if (isColor) {
                    uvLeft   = pLoadContext->paletteTextureSlot.posX + (primHeader.colorIndex % 16) / (float)pLoadContext->texturePacker.width;
                    uvBottom = pLoadContext->paletteTextureSlot.posY + (primHeader.colorIndex / 16) / (float)pLoadContext->texturePacker.height;
                    uvRight  = uvLeft;
                    uvTop    = uvBottom;
                } else {
                    uvLeft   = texture.posX / (float)pLoadContext->texturePacker.width;
                    uvBottom = texture.posY / (float)pLoadContext->texturePacker.height;
                    uvRight  = (texture.posX + texture.sizeX) / (float)pLoadContext->texturePacker.width;
                    uvTop    = (texture.posY + texture.sizeY) / (float)pLoadContext->texturePacker.height;
                }
                
                // Special case for quads because of how they are UV mapped. Not sure how UV mapping works for other polygons.
                if (primHeader.indexCount == 4)
                {
                    ta_vertex_p3t2n3 vertices[4];
                    for (int i = 0; i < 4; ++i) {
                        taInt32 position[3];
                        memcpy(position, pFile->pFileData + objectHeader.vertexPtr + (indices[i]*sizeof(taInt32)*3), sizeof(taInt32)*3);

                        // Note that the Y and Z positions are intentionally swapped.
                        vertices[i].x = position[0] / -65536.0f;
                        vertices[i].y = position[2] /  65536.0f;
                        vertices[i].z = position[1] /  65536.0f;
                    }

                    vertices[0].u = uvLeft;
                    vertices[0].v = uvBottom;
                    vertices[1].u = uvRight;
                    vertices[1].v = uvBottom;
                    vertices[2].u = uvRight;
                    vertices[2].v = uvTop;
                    vertices[3].u = uvLeft;
                    vertices[3].v = uvTop;

                    vec3 normal = vec3_triangle_normal(vec3v(&vertices[0].x), vec3v(&vertices[2].x), vec3v(&vertices[1].x));
                    for (int i = 0; i < 4; ++i) {
                        vertices[i].nx = normal.x;
                        vertices[i].ny = normal.y;
                        vertices[i].nz = normal.z;
                    }

                    ta_mesh_builder_write_vertex(pMeshBuilder, &vertices[0]);
                    ta_mesh_builder_write_vertex(pMeshBuilder, &vertices[2]);
                    ta_mesh_builder_write_vertex(pMeshBuilder, &vertices[1]);

                    ta_mesh_builder_write_vertex(pMeshBuilder, &vertices[0]);
                    ta_mesh_builder_write_vertex(pMeshBuilder, &vertices[3]);
                    ta_mesh_builder_write_vertex(pMeshBuilder, &vertices[2]);
                }
                else
                {
                    ta_vertex_p3t2n3 vertices[3];
                    memset(vertices, 0, sizeof(vertices));

                    for (taUInt32 iVertex = 0; iVertex < primHeader.indexCount-2; ++iVertex) {
                        taInt32* position0 = (taInt32*)(pFile->pFileData + objectHeader.vertexPtr + (indices[0]*sizeof(taInt32)*3));
                        taInt32* position1 = (taInt32*)(pFile->pFileData + objectHeader.vertexPtr + (indices[iVertex+1]*sizeof(taInt32)*3));
                        taInt32* position2 = (taInt32*)(pFile->pFileData + objectHeader.vertexPtr + (indices[iVertex+2]*sizeof(taInt32)*3));

                        // Note that the Y and Z positions are intentionally swapped.
                        vertices[0].x = position0[0] / -65536.0f;
                        vertices[0].y = position0[2] /  65536.0f;
                        vertices[0].z = position0[1] /  65536.0f;
                        vertices[1].x = position1[0] / -65536.0f;
                        vertices[1].y = position1[2] /  65536.0f;
                        vertices[1].z = position1[1] /  65536.0f;
                        vertices[2].x = position2[0] / -65536.0f;
                        vertices[2].y = position2[2] /  65536.0f;
                        vertices[2].z = position2[1] /  65536.0f;

                        vec3 normal = vec3_triangle_normal(vec3v(&vertices[0].x), vec3v(&vertices[2].x), vec3v(&vertices[1].x));
                        for (int i = 0; i < 3; ++i) {
                            vertices[i].nx = normal.x;
                            vertices[i].ny = normal.y;
                            vertices[i].nz = normal.z;

                            vertices[i].u = uvLeft;
                            vertices[i].v = uvTop;
                        }

                        ta_mesh_builder_write_vertex(pMeshBuilder, &vertices[0]);
                        ta_mesh_builder_write_vertex(pMeshBuilder, &vertices[2]);
                        ta_mesh_builder_write_vertex(pMeshBuilder, &vertices[1]);
                    }
                }
            } else {
                // TODO: Add support for lines and triangles. Points will need to be stored, but they shouldn't need to have a graphics representation.
                printf("Line or Point: %d\n", primHeader.indexCount);
            }
        }
    }


    // Here is where we convert the mesh builders to actual meshes. The meshes are stored in an array in the main 3DO object.
    taUInt32 objectMeshCount = (taUInt32)pLoadContext->meshBuildersCount;
    p3DO->pObjects[thisIndex].meshCount = objectMeshCount;
    p3DO->pObjects[thisIndex].firstMeshIndex = p3DO->meshCount;

    p3DO->pMeshes = (ta_map_3do_mesh*)realloc(p3DO->pMeshes, (p3DO->meshCount + objectMeshCount) * sizeof(*p3DO->pMeshes));
    if (p3DO->pMeshes == NULL) {
        return 0;
    }

    for (taUInt32 iMesh = 0; iMesh < objectMeshCount; ++iMesh) {
        ta_mesh_builder* pMeshBuilder = &pLoadContext->pMeshBuilders[iMesh];

        p3DO->pMeshes[p3DO->meshCount + iMesh].textureIndex = pMeshBuilder->textureIndex;
        p3DO->pMeshes[p3DO->meshCount + iMesh].pMesh = ta_create_mesh(pMap->pEngine->pGraphics, ta_primitive_type_triangle,
            ta_vertex_format_p3t2n3,  pMeshBuilder->vertexCount, pMeshBuilder->pVertexData,
            ta_index_format_uint32, pMeshBuilder->indexCount,  pMeshBuilder->pIndexData);
        if (p3DO->pMeshes[p3DO->meshCount + iMesh].pMesh == NULL) {
            return 0;
        }

        p3DO->pMeshes[p3DO->meshCount + iMesh].indexCount = pMeshBuilder->indexCount;
        p3DO->pMeshes[p3DO->meshCount + iMesh].indexOffset = 0; // <-- When 3DO's are constructed from larger monolithic meshes we'll want to change this.
    }

    p3DO->meshCount += objectMeshCount;



    taUInt32 childCount = 0;
    if (objectHeader.firstChildPtr != 0) {
        if (!taSeekFile(pFile, objectHeader.firstChildPtr, taSeekOriginStart)) {
            return 0;
        }

        p3DO->pObjects[thisIndex].firstChildIndex = firstChildIndex;
        childCount = ta_map__load_3do_objects_recursive(pMap, pLoadContext, pFile, p3DO, firstChildIndex);
        if (childCount == 0) {
            return 0;   // An error occured.
        }
    } else {
        p3DO->pObjects[thisIndex].firstChildIndex = 0;
    }
    
    taUInt32 siblingCount = 0;
    taUInt32 nextSiblingIndex = firstChildIndex + childCount;
    if (objectHeader.nextSiblingPtr != 0) {
        if (!taSeekFile(pFile, objectHeader.nextSiblingPtr, taSeekOriginStart)) {
            return 0;
        }

        p3DO->pObjects[thisIndex].nextSiblingIndex = nextSiblingIndex;
        siblingCount = ta_map__load_3do_objects_recursive(pMap, pLoadContext, pFile, p3DO, nextSiblingIndex);
        if (siblingCount == 0) {
            return 0;   // An error occured.
        }
    } else {
        p3DO->pObjects[thisIndex].nextSiblingIndex = 0;
    }
    
    return 1 + childCount + siblingCount;
}

ta_map_3do* ta_map__load_3do(ta_map_instance* pMap, ta_map_load_context* pLoadContext, const char* objectName)
{
    // 3DO files are in the "objects3d" folder.
    char fullFileName[TA_MAX_PATH];
    if (!drpath_copy_and_append(fullFileName, sizeof(fullFileName), "objects3d", objectName /*"armgate"*/)) {
        return NULL;
    }
    if (!drpath_extension_equal(objectName, "3do")) {
        if (!drpath_append_extension(fullFileName, sizeof(fullFileName), "3do")) {
            return NULL;
        }
    }

    taFile* pFile = taOpenFile(pMap->pEngine->pFS, fullFileName, 0);
    if (pFile == NULL) {
        return NULL;
    }

    taUInt32 objectCount = ta3DOCountObjects(pFile);
    if (objectCount == 0) {
        taCloseFile(pFile);
        return NULL;
    }

    ta_map_3do* p3DO = (ta_map_3do*)malloc(sizeof(*p3DO));
    if (p3DO == NULL) {
        taCloseFile(pFile);
        return NULL;
    }

    p3DO->meshCount = 0;
    p3DO->pMeshes = NULL;

    p3DO->objectCount = objectCount;
    p3DO->pObjects = (ta_map_3do_object*)malloc(objectCount * sizeof(*p3DO->pObjects));
    if (p3DO->pObjects == NULL) {
        taCloseFile(pFile);
        free(p3DO);
        return NULL;
    }

    taSeekFile(pFile, 0, taSeekOriginStart);

    taUInt32 objectsLoaded = ta_map__load_3do_objects_recursive(pMap, pLoadContext, pFile, p3DO, 0);
    if (objectsLoaded != p3DO->objectCount) {
        taCloseFile(pFile);
        free(p3DO->pMeshes);
        free(p3DO->pObjects);
        free(p3DO);
        return NULL;
    }

    taCloseFile(pFile);
    return p3DO;
}


void ta_map__calculate_object_position_xy(taUInt32 tileX, taUInt32 tileY, taUInt16 objectFootprintX, taUInt16 objectFootprintY, float* pPosXOut, float* pPosYOut)
{
    assert(pPosXOut != NULL);
    assert(pPosYOut != NULL);

    float tileCenterX = (tileX * 16.0f);// + 8.0f;
    float tileCenterY = (tileY * 16.0f);// + 8.0f;

    *pPosXOut = tileCenterX + (objectFootprintX/2 * 16);
    *pPosYOut = tileCenterY + (objectFootprintY/2 * 16);
}


taFile* ta_map__open_tnt_file(taFS* pFS, const char* mapName)
{
    char filename[TA_MAX_PATH];
    if (!drpath_copy_and_append(filename, sizeof(filename), "maps", mapName)) {
        return NULL;
    }
    if (!drpath_append_extension(filename, sizeof(filename), "tnt")) {
        return NULL;
    }

    return taOpenFile(pFS, filename, 0);
}

void ta_map__close_tnt_file(taFile* pTNT)
{
    taCloseFile(pTNT);
}

taBool32 ta_map__read_tnt_header(taFile* pTNT, ta_tnt_header* pHeader)
{
    assert(pTNT != NULL);
    assert(pHeader != NULL);

    if (!taReadFileUInt32(pTNT, &pHeader->id)) {
        return TA_FALSE;
    }

    if (pHeader->id != 8192) {
        return TA_FALSE;   // Not a TNT file.
    }


    if (!taReadFileUInt32(pTNT, &pHeader->width) ||
        !taReadFileUInt32(pTNT, &pHeader->height) ||
        !taReadFileUInt32(pTNT, &pHeader->mapdataPtr) ||
        !taReadFileUInt32(pTNT, &pHeader->mapattrPtr) ||
        !taReadFileUInt32(pTNT, &pHeader->tilegfxPtr) ||
        !taReadFileUInt32(pTNT, &pHeader->tileCount) ||
        !taReadFileUInt32(pTNT, &pHeader->featureTypesCount) ||
        !taReadFileUInt32(pTNT, &pHeader->featureTypesPtr) ||
        !taReadFileUInt32(pTNT, &pHeader->seaLevel) ||
        !taReadFileUInt32(pTNT, &pHeader->minimapPtr) ||
        !taSeekFile(pTNT, 20, taSeekOriginCurrent))    // <-- Last 20 bytes are unused.
    {
        return TA_FALSE;
    }

    return TA_TRUE;
}

taBool32 ta_map__load_tnt(ta_map_instance* pMap, const char* mapName, ta_map_load_context* pLoadContext)
{
    assert(pMap != NULL);
    assert(mapName != NULL);
    assert(pLoadContext != NULL);

    taFile* pTNT = ta_map__open_tnt_file(pMap->pEngine->pFS, mapName);
    if (pTNT == NULL) {
        return TA_FALSE;
    }

    // Header.
    ta_tnt_header header;
    if (!ta_map__read_tnt_header(pTNT, &header)) {
        return TA_FALSE;
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
    taUInt32 tilesPerChunk = TA_TERRAIN_CHUNK_SIZE*TA_TERRAIN_CHUNK_SIZE;
    taUInt32 totalChunkCount = pMap->terrain.chunkCountX * pMap->terrain.chunkCountY;
    taUInt32 totalTileCount = tilesPerChunk * totalChunkCount;

    pMap->terrain.pChunks = calloc(totalChunkCount, sizeof(*pMap->terrain.pChunks));
    if (pMap->terrain.pChunks == NULL) {
        goto on_error;
    }

    ta_vertex_p2t2* pVertexData = malloc(totalTileCount*4 * sizeof(ta_vertex_p2t2));
    if (pVertexData == NULL) {
        goto on_error;
    }

    taUInt32* pIndexData = malloc(totalTileCount*4 * sizeof(taUInt32));
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
    if (!taSeekFile(pTNT, header.tilegfxPtr, taSeekOriginStart)) {
        free(pIndexData);
        free(pVertexData);
        free(pTileSubImages);
        goto on_error;
    }

    for (taUInt32 iTile = 0; iTile < header.tileCount; /* DO NOTHING */)
    {
        char* pTileImageData = pTNT->pFileData + taTellFile(pTNT) + (iTile*32*32);

        ta_texture_packer_slot slot;
        if (ta_texture_packer_pack_subtexture(&pLoadContext->texturePacker, 32, 32, pTileImageData, &slot))
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
            if (!ta_map__create_and_push_texture(pMap, &pLoadContext->texturePacker)) {  // <-- This will reset the texture packer.
                free(pIndexData);
                free(pVertexData);
                free(pTileSubImages);
                goto on_error;
            }
        }
    }


    
    // For every chunk...
    for (taUInt32 chunkY = 0; chunkY < pMap->terrain.chunkCountY; ++chunkY)
    {
        taUInt32 chunkTileCountY = TA_TERRAIN_CHUNK_SIZE;
        if ((chunkY*TA_TERRAIN_CHUNK_SIZE) + chunkTileCountY > pMap->terrain.tileCountY) {
            chunkTileCountY = pMap->terrain.tileCountY - (chunkY*TA_TERRAIN_CHUNK_SIZE);
        }

        for (taUInt32 chunkX = 0; chunkX < pMap->terrain.chunkCountX; ++chunkX)
        {
            taUInt32 chunkTileCountX = TA_TERRAIN_CHUNK_SIZE;
            if ((chunkX*TA_TERRAIN_CHUNK_SIZE) + chunkTileCountX > pMap->terrain.tileCountX) {
                chunkTileCountX = pMap->terrain.tileCountX - (chunkX*TA_TERRAIN_CHUNK_SIZE);
            }


            ta_map_terrain_chunk* pChunk = pMap->terrain.pChunks + ((chunkY*pMap->terrain.chunkCountX) + chunkX);

            taUInt32 chunkVertexOffset = ((chunkY*pMap->terrain.chunkCountX) + chunkX) * (tilesPerChunk*4);
            taUInt32 chunkIndexOffset  = ((chunkY*pMap->terrain.chunkCountX) + chunkX) * (tilesPerChunk*4);


            // We want to group each mesh in the chunk by texture...
            for (taUInt32 iTexture = 0; iTexture < pMap->textureCount+1; ++iTexture)    // <-- +1 because there is a texture sitting in the packer that hasn't yet been added to the list.
            {
                taBool32 isMeshAllocatedForThisTextures = TA_FALSE;
                
                // For every tile in the chunk...
                for (taUInt32 tileY = 0; tileY < chunkTileCountY; ++tileY)
                {
                    // Seek to the first tile in this row.
                    taUInt32 firstTileOnRow = ((chunkY*TA_TERRAIN_CHUNK_SIZE + tileY) * pMap->terrain.tileCountX) + (chunkX*TA_TERRAIN_CHUNK_SIZE);
                    if (!taSeekFile(pTNT, header.mapdataPtr + (firstTileOnRow * sizeof(taUInt16)), taSeekOriginStart)) {
                        free(pIndexData);
                        free(pVertexData);
                        free(pTileSubImages);
                        goto on_error;
                    }

                    for (taUInt32 tileX = 0; tileX < chunkTileCountX; ++tileX)
                    {
                        taUInt16 tileIndex;
                        if (!taReadFileUInt16(pTNT, &tileIndex)) {
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

                                isMeshAllocatedForThisTextures = TA_TRUE;
                            }
                            
                            // We always add the vertex to the most recent mesh.
                            ta_map_terrain_submesh* pMesh = pChunk->pMeshes + (pChunk->meshCount - 1);
                            
                            ta_vertex_p2t2* pQuad = pVertexData + chunkVertexOffset;

                            float tileU = pTileSubImages[tileIndex].posX / (float)pLoadContext->texturePacker.width;
                            float tileV = pTileSubImages[tileIndex].posY / (float)pLoadContext->texturePacker.height;
                            
                            // Top left.
                            pQuad[0].x = (float)(chunkX*TA_TERRAIN_CHUNK_SIZE + tileX) * 32.0f;
                            pQuad[0].y = (float)(chunkY*TA_TERRAIN_CHUNK_SIZE + tileY) * 32.0f;
                            pQuad[0].u = tileU;
                            pQuad[0].v = tileV;

                            // Bottom left
                            pQuad[1].x = pQuad[0].x;
                            pQuad[1].y = pQuad[0].y + 32;
                            pQuad[1].u = pQuad[0].u;
                            pQuad[1].v = pQuad[0].v + (32.0f / pLoadContext->texturePacker.height);

                            // Bottom right
                            pQuad[2].x = pQuad[1].x + 32;
                            pQuad[2].y = pQuad[1].y;
                            pQuad[2].u = pQuad[1].u + (32.0f / pLoadContext->texturePacker.width);
                            pQuad[2].v = pQuad[1].v;

                            // Top right
                            pQuad[3].x = pQuad[2].x;
                            pQuad[3].y = pQuad[2].y - 32;
                            pQuad[3].u = pQuad[2].u;
                            pQuad[3].v = pQuad[2].v - (32.0f / pLoadContext->texturePacker.height);



                            taUInt32* pQuadIndices = pIndexData + chunkIndexOffset;
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
    pMap->terrain.pMesh = ta_create_mesh(pMap->pEngine->pGraphics, ta_primitive_type_quad, ta_vertex_format_p2t2, totalTileCount*4, pVertexData, ta_index_format_uint32, totalTileCount*4, pIndexData);
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
    if (!taSeekFile(pTNT, header.featureTypesPtr, taSeekOriginStart)) {
        goto on_error;
    }

    pMap->featureTypesCount = header.featureTypesCount;
    pMap->pFeatureTypes = calloc(header.featureTypesCount, sizeof(*pMap->pFeatureTypes));
    if (pMap->pFeatureTypes == NULL) {
        goto on_error;
    }

    for (taUInt32 iFeatureType = 0; iFeatureType < pMap->featureTypesCount; ++iFeatureType) {
        if (!taSeekFile(pTNT, 4, taSeekOriginCurrent)) {
            goto on_error;
        }

        if (!taReadFile(pTNT, pMap->pFeatureTypes[iFeatureType].name, 128, NULL)) {
            goto on_error;
        }

        pMap->pFeatureTypes[iFeatureType].pDesc = taFindFeatureDesc(pMap->pEngine->pFeatures, pMap->pFeatureTypes[iFeatureType].name);
        pMap->pFeatureTypes[iFeatureType]._index = iFeatureType;
        //printf("%s\n", pMap->pFeatureTypes[iFeatureType].pDesc->filename);
    }

    // The feature types need to be sorted by their file name so we can load them efficiently.
    qsort(pMap->pFeatureTypes, pMap->featureTypesCount, sizeof(*pMap->pFeatureTypes), ta_map__sort_feature_types_by_filename);


    ta_gaf* pCurrentGAF = NULL;
    for (taUInt32 iFeatureType = 0; iFeatureType < pMap->featureTypesCount; ++iFeatureType)
    {
        ta_map_feature_type* pFeatureType = &pMap->pFeatureTypes[iFeatureType];
        if (pFeatureType->pDesc->filename[0] != '\0')
        {
            if (pCurrentGAF == NULL || _stricmp(pCurrentGAF->filename, pFeatureType->pDesc->filename) != 0)
            {
                // A new GAF file needs to be loaded. These will be in the "anims" directory.
                char filename[TA_MAX_PATH];
                if (!drpath_copy_and_append(filename, sizeof(filename), "anims", pFeatureType->pDesc->filename)) {
                    goto on_error;
                }

                ta_close_gaf(pCurrentGAF);
                pCurrentGAF = ta_open_gaf(pMap->pEngine->pFS, filename);
                if (pCurrentGAF == NULL) {
                    goto on_error;
                }
            }

            // At this point the GAF file containing the feature should be loaded and we just need to read it's frame data for
            // every required sequence.
            pFeatureType->pSequenceDefault = ta_map__load_gaf_sequence(pMap, &pLoadContext->texturePacker, pCurrentGAF, pFeatureType->pDesc->seqname);
            pFeatureType->pSequenceBurn = ta_map__load_gaf_sequence(pMap, &pLoadContext->texturePacker, pCurrentGAF, pFeatureType->pDesc->seqnameburn);
            pFeatureType->pSequenceDie = ta_map__load_gaf_sequence(pMap, &pLoadContext->texturePacker, pCurrentGAF, pFeatureType->pDesc->seqnamedie);
            pFeatureType->pSequenceReclamate = ta_map__load_gaf_sequence(pMap, &pLoadContext->texturePacker, pCurrentGAF, pFeatureType->pDesc->seqnamereclamate);
            pFeatureType->pSequenceShadow = ta_map__load_gaf_sequence(pMap, &pLoadContext->texturePacker, pCurrentGAF, pFeatureType->pDesc->seqnameshadow);
        }
        else
        {
            // It's not a 2D feature so assume it's a 3D one.
            pFeatureType->p3DO = ta_map__load_3do(pMap, pLoadContext, pFeatureType->pDesc->object);
            if (pFeatureType->p3DO == NULL) {
                goto on_error;
            }
        }
    }

    ta_close_gaf(pCurrentGAF);



    // Features are loaded by iterating over each 16x16 tile. The type of each feature is determine based on an index, however
    // remember from earlier that we sorted the features which means those indexes are no longer valid. To address this we just
    // sort it back to it's original order.
    qsort(pMap->pFeatureTypes, pMap->featureTypesCount, sizeof(*pMap->pFeatureTypes), ta_map__sort_feature_types_by_index);

    if (!taSeekFile(pTNT, header.mapattrPtr, taSeekOriginStart)) {
        goto on_error;
    }

    taUInt32 featureCount = 0;
    for (taUInt32 y = 0; y < header.height; ++y) {
        for (taUInt32 x = 0; x < header.width; ++x) {
            taUInt32 tile;
            if (!taReadFileUInt32(pTNT, &tile)) {
                goto on_error;
            }

            taUInt16 featureTypeIndex = (taUInt16)((tile & 0x00FFFF00) >> 8);
            if (featureTypeIndex != 0xFFFF && featureTypeIndex != 0xFFFC && featureTypeIndex != 0xFFFE) {
                featureCount += 1;
            }
        }
    }


    if (!taSeekFile(pTNT, header.mapattrPtr, taSeekOriginStart)) {
        goto on_error;
    }

    pMap->pFeatures = malloc(featureCount * sizeof(*pMap->pFeatures));
    if (pMap->pFeatures == NULL) {
        goto on_error;
    }

    pMap->featureCount = 0;
    for (taUInt32 y = 0; y < header.height; ++y) {
        for (taUInt32 x = 0; x < header.width; ++x) {
            taUInt32 tile;
            if (!taReadFileUInt32(pTNT, &tile)) {
                goto on_error;
            }

            taUInt8 tileHeight = (taUInt8)((tile & 0xFF));

            taUInt16 featureTypeIndex = (taUInt16)((tile & 0x00FFFF00) >> 8);
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
    return TA_TRUE;


on_error:
    ta_map__close_tnt_file(pTNT);
    return TA_FALSE;
}


taConfigObj* ta_map__open_ota_file(taFS* pFS, const char* mapName)
{
    char filename[TA_MAX_PATH];
    if (!drpath_copy_and_append(filename, sizeof(filename), "maps", mapName)) {
        return NULL;
    }
    if (!drpath_append_extension(filename, sizeof(filename), "ota")) {
        return NULL;
    }

    return taParseConfigFromFile(pFS, filename);
}

void ta_map__close_ota_file(taConfigObj* pOTA)
{
    taDeleteConfig(pOTA);
}

taBool32 ta_map__load_ota(ta_map_instance* pMap, const char* mapName)
{
    taConfigObj* pOTA = ta_map__open_ota_file(pMap->pEngine->pFS, mapName);
    if (pOTA == NULL) {
        return TA_FALSE;
    }

    // TODO: Do something.

    ta_map__close_ota_file(pOTA);
    return TA_TRUE;
}


taBool32 ta_map_load_context_init(ta_map_load_context* pLoadContext, taEngineContext* pEngine)
{
    if (pLoadContext == NULL || pEngine == NULL) {
        return TA_FALSE;
    }

    memset(pLoadContext, 0, sizeof(*pLoadContext));

    // Clamp the texture size to avoid excessive wastage. Modern GPUs support 16K textures which is way more than we need, and
    // I'd rather avoid wasting the player's system resources.
    taUInt32 maxTextureSize = ta_get_max_texture_size(pEngine->pGraphics);
    if (maxTextureSize > TA_MAX_TEXTURE_ATLAS_SIZE) {
        maxTextureSize = TA_MAX_TEXTURE_ATLAS_SIZE;
    }

    // We'll need a texture packer to help us pack images into atlases.
    if (!ta_texture_packer_init(&pLoadContext->texturePacker, maxTextureSize, maxTextureSize, 1, 0)) {
        return TA_FALSE;
    }

    return TA_TRUE;
}

void ta_map_load_context_uninit(ta_map_load_context* pLoadContext)
{
    if (pLoadContext == NULL) {
        return;
    }

    ta_texture_packer_uninit(&pLoadContext->texturePacker);
}


ta_map_instance* ta_load_map(taEngineContext* pEngine, const char* mapName)
{
    if (pEngine == NULL || mapName == NULL) {
        return NULL;
    }

    ta_map_load_context loadContext;
    if (!ta_map_load_context_init(&loadContext, pEngine)) {
        return NULL;
    }

    ta_map_instance* pMap = calloc(1, sizeof(*pMap));
    if (pMap == NULL) {
        ta_map_load_context_uninit(&loadContext);
        return NULL;
    }

    pMap->pEngine = pEngine;

    // The first 16x16 texture needs to be set to the palette.
    taUInt8 paletteIndices[256];
    for (int i = 0; i < 256; ++i) {
        paletteIndices[i] = (taUInt8)i;
    }

    ta_map__pack_subtexture(pMap, &loadContext.texturePacker, 16, 16, paletteIndices, &loadContext.paletteTextureSlot);

    if (!ta_map__load_tnt(pMap, mapName, &loadContext)) {
        goto on_error;
    }

    if (!ta_map__load_ota(pMap, mapName)) {
        goto on_error;
    }

    
    // At the end of loading everything there could be a texture still sitting in the packer which needs to be created.
    if (loadContext.texturePacker.cursorPosX != 0 || loadContext.texturePacker.cursorPosY != 0) {
        if (!ta_map__create_and_push_texture(pMap, &loadContext.texturePacker)) {  // <-- This will reset the texture packer.
            goto on_error;
        }
    }
    

    ta_map_load_context_uninit(&loadContext);
    return pMap;


on_error:
    ta_map_load_context_uninit(&loadContext);
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