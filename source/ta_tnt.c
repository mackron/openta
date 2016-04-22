// Public domain. See "unlicense" statement at the end of this file.

// Credits to http://units.tauniverse.com/tutorials/tadesign/tadesign/ta-tnt-fmt.txt for the explanation of this file format.

// The number of tiles making up a chunk, on each dimension.
#define TA_TNT_CHUNK_SIZE   16

ta_tnt* ta_load_tnt_from_file(ta_hpi_file* pFile, ta_graphics_context* pGraphics)
{
    if (pFile == NULL) {
        return NULL;
    }

    uint32_t id;
    if (!ta_hpi_read(pFile, &id, 4, NULL)) {
        return NULL;
    }

    if (id != 8192) {
        return NULL;    // Not a TNT file.
    }


    uint32_t width;
    if (!ta_hpi_read(pFile, &width, 4, NULL)) {
        return NULL;
    }

    uint32_t height;
    if (!ta_hpi_read(pFile, &height, 4, NULL)) {
        return NULL;
    }


    uint32_t mapdataPtr;
    if (!ta_hpi_read(pFile, &mapdataPtr, 4, NULL)) {
        return NULL;
    }

    uint32_t mapattrPtr;
    if (!ta_hpi_read(pFile, &mapattrPtr, 4, NULL)) {
        return NULL;
    }

    uint32_t tilegfxPtr;
    if (!ta_hpi_read(pFile, &tilegfxPtr, 4, NULL)) {
        return NULL;
    }

    uint32_t tileCount;
    if (!ta_hpi_read(pFile, &tileCount, 4, NULL)) {
        return NULL;
    }

    uint32_t specialItemsCount;
    if (!ta_hpi_read(pFile, &specialItemsCount, 4, NULL)) {
        return NULL;
    }

    uint32_t specialItemsPtr;
    if (!ta_hpi_read(pFile, &specialItemsPtr, 4, NULL)) {
        return NULL;
    }

    int32_t seaLevel;
    if (!ta_hpi_read(pFile, &seaLevel, 4, NULL)) {
        return NULL;
    }

    uint32_t minimapPtr;
    if (!ta_hpi_read(pFile, &minimapPtr, 4, NULL)) {
        return NULL;
    }

    uint32_t unknown;
    if (!ta_hpi_read(pFile, &unknown, 4, NULL)) {
        return NULL;
    }

    uint32_t padding[4];
    if (!ta_hpi_read(pFile, padding, 4*4, NULL)) {
        return NULL;
    }


    ta_tnt* pTNT = calloc(1, sizeof(*pTNT));
    if (pTNT == NULL) {
        return NULL;
    }

    // The width and height needs to be divided by two for some reason. Not sure on the rationale for that...
    pTNT->width = width/2;
    pTNT->height = height/2;
    pTNT->seaLevel = seaLevel;


    //// Graphics ////

    // The map's graphics are defined by 32x32 tiles. The first thing to do is build texture atlases to contain this data. Because each tile
    // is 32x32, we have a very tightly fitting group of texture atlases, each of which have a very specific number of tiles that can fit
    // within it. This property allows us to very easily determine which texture atlas a tile's graphics is contained in.
    //
    // Texture atlases are clamped to 1024x1024 in order to avoid too much waste, while also keeping the number of atlases down.
    if (!ta_hpi_seek(pFile, tilegfxPtr, ta_hpi_seek_origin_start)) {
        goto on_error;
    }


    uint32_t atlasSize = ta_get_max_texture_size(pGraphics);
    if (atlasSize > 1024) {
        atlasSize = 1024;
    }

    uint32_t tilesPerAtlasRow = atlasSize / 32;
    uint32_t tilesPerAtlasCol = tilesPerAtlasRow;   // <-- Always square.
    uint32_t tilesPerAtlas = tilesPerAtlasRow * tilesPerAtlasCol;

    pTNT->textureCount = tileCount / tilesPerAtlas;
    if (tileCount % tilesPerAtlas > 0) {
        pTNT->textureCount += 1;
    }

    pTNT->ppTextures = calloc(pTNT->textureCount, sizeof(*pTNT->ppTextures));
    if (pTNT->ppTextures == NULL) {
        goto on_error;
    }

    uint8_t* pTextureData = malloc(atlasSize*atlasSize);
    if (pTextureData == NULL) {
        free(pTextureData);
        goto on_error;
    }

    

    uint8_t pTileData[32*32];

    uint32_t tilesRemaining = tileCount;
    for (uint32_t iTexture = 0; iTexture < pTNT->textureCount; ++iTexture)
    {
        // Clear the texture data for debugging.
        memset(pTextureData, 0, atlasSize*atlasSize);

        uint32_t tilesInThisAtlas = dr_min(tilesPerAtlas, tilesRemaining);
        for (uint32_t iTile = 0; iTile < tilesInThisAtlas; ++iTile)
        {
            uint32_t atlasTileRow = iTile / tilesPerAtlasCol;
            uint32_t atlasTileCol = iTile - (atlasTileRow * tilesPerAtlasCol);

            uint32_t atlasX = atlasTileCol * 32;
            uint32_t atlasY = atlasTileRow * 32;

            if (!ta_hpi_read(pFile, pTileData, sizeof(pTileData), NULL)) {
                free(pTextureData);
                goto on_error;
            }

            // Copy into the texture atlas.
            for (uint32_t y = 0; y < 32; ++y) {
                for (uint32_t x = 0; x < 32; ++x) {
                    pTextureData[((atlasY + y) * atlasSize) + (atlasX + x)] = pTileData[(y*32) + x];
                }
            }
        }

        tilesRemaining -= tilesInThisAtlas;


        // At this point all we need to do is create the texture atlas on the graphics system.
        pTNT->ppTextures[iTexture] = ta_create_texture(pGraphics, atlasSize, atlasSize, 1, pTextureData);
        if (pTNT->ppTextures[iTexture] == NULL) {
            free(pTextureData);
            goto on_error;
        }
    }

    free(pTextureData);


    // At this point we have the texture atlases generated so now we need to build the meshes that will be used to draw
    // the map. Each mesh will be assigned a single texture, and there will be multiple meshes making up the whole map.
    //
    // The map's geometry is divided up into chunks, with each chunk being made up of 16x16 tiles. This subdivision is
    // used so the engine can cull any chunks that aren't visible on the screen during rendering.

    pTNT->chunkCountX = pTNT->width / TA_TNT_CHUNK_SIZE;
    if ((pTNT->chunkCountX % TA_TNT_CHUNK_SIZE) > 0) {
        pTNT->chunkCountX += 1;
    }

    pTNT->chunkCountY = pTNT->height / TA_TNT_CHUNK_SIZE;
    if ((pTNT->chunkCountY % TA_TNT_CHUNK_SIZE) > 0) {
        pTNT->chunkCountY += 1;
    }

    pTNT->pChunks = calloc(pTNT->chunkCountX*pTNT->chunkCountY, sizeof(*pTNT->pChunks));
    if (pTNT->pChunks == NULL) {
        goto on_error;
    }

    // For every chunk...
    for (uint32_t chunkY = 0; chunkY < pTNT->chunkCountY; ++chunkY) {
        for (uint32_t chunkX = 0; chunkX < pTNT->chunkCountX; ++chunkX) {
            ta_tnt_tile_chunk* pChunk = pTNT->pChunks + ((chunkY*pTNT->chunkCountX) + chunkX);

            // Create a sub-chunk for each texture atlas. Can likely optimize this because this is worst-case.
            pChunk->subchunkCount = pTNT->textureCount;
            pChunk->pSubchunks = calloc(pChunk->subchunkCount, sizeof(*pChunk->pSubchunks));

            for (uint32_t iSubchunk = 0; iSubchunk < pChunk->subchunkCount; ++iSubchunk) {
                pChunk->pSubchunks[iSubchunk].pTexture = pTNT->ppTextures[iSubchunk];
                pChunk->pSubchunks[iSubchunk].pMesh = malloc((sizeof(ta_tnt_mesh) - sizeof(ta_tnt_mesh_vertex)) + (sizeof(ta_tnt_mesh_vertex) * TA_TNT_CHUNK_SIZE*TA_TNT_CHUNK_SIZE * 4));   // 4 = one vertex for each quad.
                pChunk->pSubchunks[iSubchunk].pMesh->quadCount = 0;
            }


            // For every tile in the chunk.
            for (uint32_t tileY = 0; tileY < TA_TNT_CHUNK_SIZE; ++tileY) {
                if ((chunkY*TA_TNT_CHUNK_SIZE) + tileY >= pTNT->height) {
                    break;
                }

                // Seek to the first tile in this row.
                uint32_t firstTileOnRow = ((chunkY*TA_TNT_CHUNK_SIZE + tileY) * pTNT->width) + (chunkX*TA_TNT_CHUNK_SIZE);
                if (!ta_hpi_seek(pFile, mapdataPtr + (firstTileOnRow * sizeof(uint16_t)), ta_hpi_seek_origin_start)) {
                    goto on_error;
                }

                for (uint32_t tileX = 0; tileX < TA_TNT_CHUNK_SIZE; ++tileX) {
                    if ((chunkX*TA_TNT_CHUNK_SIZE) + tileX >= pTNT->width) {
                        break;
                    }

                    uint16_t tileIndex;
                    if (!ta_hpi_read(pFile, &tileIndex, 2, NULL)) {
                        goto on_error;
                    }

                    // From the tile index we can determine which texture atlast to use and which subchunk to use to draw this tile. Once we have
                    // determined which subchunk to add it to we just add the quad to the list.
                    uint16_t iSubchunk = tileIndex / tilesPerAtlas;
                    uint16_t tileIndexInAtlas = tileIndex - (iSubchunk * tilesPerAtlas);

                    uint32_t tileV = tileIndexInAtlas / tilesPerAtlasRow;
                    uint32_t tileU = tileIndexInAtlas - (tileV * tilesPerAtlasRow);


                    ta_tnt_mesh* pMesh = pChunk->pSubchunks[iSubchunk].pMesh;
                    uint32_t quadIndex = pMesh->quadCount;

                    ta_tnt_mesh_vertex* pQuad = pMesh->pVertices + (quadIndex*4);
                    
                    // Top left
                    pQuad[0].x = (float)(chunkX*TA_TNT_CHUNK_SIZE + tileX) * 32.0f;
                    pQuad[0].y = (float)(chunkY*TA_TNT_CHUNK_SIZE + tileY) * 32.0f;
                    pQuad[0].u = (float)(tileU*32.0f) / pChunk->pSubchunks[iSubchunk].pTexture->width;
                    pQuad[0].v = (float)(tileV*32.0f) / pChunk->pSubchunks[iSubchunk].pTexture->height;

                    // Bottom left
                    pQuad[1].x = pQuad[0].x;
                    pQuad[1].y = pQuad[0].y + 32;
                    pQuad[1].u = pQuad[0].u;
                    pQuad[1].v = pQuad[0].v + (32.0f / pChunk->pSubchunks[iSubchunk].pTexture->height);

                    // Bottom right
                    pQuad[2].x = pQuad[1].x + 32;
                    pQuad[2].y = pQuad[1].y;
                    pQuad[2].u = pQuad[1].u + (32.0f / pChunk->pSubchunks[iSubchunk].pTexture->height);
                    pQuad[2].v = pQuad[1].v;

                    // Top right
                    pQuad[3].x = pQuad[2].x;
                    pQuad[3].y = pQuad[2].y - 32;
                    pQuad[3].u = pQuad[2].u;
                    pQuad[3].v = pQuad[2].v - (32.0f / pChunk->pSubchunks[iSubchunk].pTexture->height);

                    pMesh->quadCount += 1;
                }
            }
        }
    }





    //// Minimap ////

    // The minimap is a maximum of 252 x 252. We will use a 256x256 texture to keep it a power of 2.
    if (!ta_hpi_seek(pFile, minimapPtr, ta_hpi_seek_origin_start)) {
        return NULL;
    }

    if (!ta_hpi_read(pFile, &pTNT->minimapWidth, 4, NULL)) {
        return NULL;
    }

    if (!ta_hpi_read(pFile, &pTNT->minimapHeight, 4, NULL)) {
        return NULL;
    }

    uint32_t minimapTextureWidth  = ta_next_power_of_2(pTNT->minimapWidth);
    uint32_t minimapTextureHeight = ta_next_power_of_2(pTNT->minimapHeight);

    uint8_t* pMinimapImageData = calloc(1, minimapTextureWidth*minimapTextureHeight);
    if (!pMinimapImageData) {
        goto on_error;
    }

    // Need to read row-by-row because the size of the minimap image is different to the texture. Also, we need to
    // flip the image data because OpenGL...
    for (uint32_t y = 0; y < pTNT->minimapHeight; ++y) {
        uint8_t* pRow = pMinimapImageData + (y*minimapTextureWidth);
        if (!ta_hpi_read(pFile, pRow, pTNT->minimapWidth, NULL)) {
            free(pMinimapImageData);
            goto on_error;
        }

        // The sizing of the minimap is strange. The value specied in the file appears to always be equal to 252, however the actual minimap
        // isn't always this size. When it's not, it'll be padded with what appears to be 0x64. What we'll do is check if the entire row is
        // equal to this value, and if so, treat that as our indicator that the minimap finishes at this point.
        bool atEnd = true;
        for (uint32_t x = 0; x < pTNT->minimapWidth; ++x) {
            if (pRow[x] != 0x64) {
                atEnd = false;
                break;
            }
        }

        if (atEnd) {
            pTNT->minimapHeight = y;
            break;
        }

        // OPTIONAL: Clear the unused area.
    }

    pTNT->pMinimapTexture = ta_create_texture(pGraphics, minimapTextureWidth, minimapTextureHeight, 1, pMinimapImageData);

    free(pMinimapImageData);
    return pTNT;

on_error:
    ta_unload_tnt(pTNT);
    return NULL;
}

void ta_unload_tnt(ta_tnt* pTNT)
{
    if (pTNT == NULL) {
        return;
    }

    if (pTNT->ppTextures != NULL) {
        for (uint32_t iTexture = 0; iTexture < pTNT->textureCount; ++iTexture) {
            ta_delete_texture(pTNT->ppTextures[iTexture]);
        }

        free(pTNT->ppTextures);
    }

    if (pTNT->pChunks != NULL) {
        for (uint32_t iChunk = 0; iChunk < (pTNT->chunkCountX*pTNT->chunkCountY); ++iChunk) {
            if (pTNT->pChunks[iChunk].pSubchunks) {
                for (uint32_t iSubchunk = 0; iSubchunk < pTNT->pChunks[iChunk].subchunkCount; ++iSubchunk) {
                    if (pTNT->pChunks[iChunk].pSubchunks[iSubchunk].pMesh) {
                        free(pTNT->pChunks[iChunk].pSubchunks[iSubchunk].pMesh);
                    }
                }

                free(pTNT->pChunks[iChunk].pSubchunks);
            }
        }

        free(pTNT->pChunks);
    }

    free(pTNT);
}