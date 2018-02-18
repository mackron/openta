// Copyright (C) 2018 David Reid. See included LICENSE file.

taBool32 ta_mesh_builder_init(ta_mesh_builder* pBuilder, size_t vertexSize)
{
    if (pBuilder == NULL || vertexSize == 0) {
        return TA_FALSE;
    }

    dr_zero_object(pBuilder);
    pBuilder->vertexSize = vertexSize;

    return TA_TRUE;
}

void ta_mesh_builder_uninit(ta_mesh_builder* pBuilder)
{
    if (pBuilder == NULL) {
        return;
    }

    free(pBuilder->pVertexData);
    free(pBuilder->pIndexData);
}


taBool32 ta_mesh_builder_write_vertex(ta_mesh_builder* pBuilder, const void* pVertexData)
{
    if (pBuilder == NULL || pVertexData == NULL) {
        return TA_FALSE;
    }

    // Index.
    if (pBuilder->indexCount == pBuilder->indexBufferSize)
    {
        size_t newIndexBufferSize = (pBuilder->indexBufferSize == 0) ? 128 : pBuilder->indexBufferSize*2;
        void* pNewIndexData = realloc(pBuilder->pIndexData, newIndexBufferSize * sizeof(taUInt32));
        if (pNewIndexData == NULL) {
            return TA_FALSE;
        }

        pBuilder->indexBufferSize = newIndexBufferSize;
        pBuilder->pIndexData = pNewIndexData;
    }

    assert(pBuilder->indexCount < pBuilder->indexBufferSize);

    pBuilder->pIndexData[pBuilder->indexCount] = pBuilder->vertexCount;
    pBuilder->indexCount += 1;


    // Vertex.
    if (pBuilder->vertexCount == pBuilder->vertexBufferSize)
    {
        size_t newVertexBufferSize = (pBuilder->vertexBufferSize == 0) ? 128 : pBuilder->vertexBufferSize*2;
        void* pNewVertexData = realloc(pBuilder->pVertexData, newVertexBufferSize * pBuilder->vertexSize);
        if (pNewVertexData == NULL) {
            return TA_FALSE;
        }

        pBuilder->vertexBufferSize = newVertexBufferSize;
        pBuilder->pVertexData = pNewVertexData;
    }

    assert(pBuilder->vertexCount < pBuilder->vertexBufferSize);

    memcpy((taUInt8*)pBuilder->pVertexData + pBuilder->vertexCount*pBuilder->vertexSize, pVertexData, pBuilder->vertexSize);
    pBuilder->vertexCount += 1;

    return TA_TRUE;
}


void ta_mesh_builder_reset(ta_mesh_builder* pBuilder)
{
    if (pBuilder == NULL) {
        return;
    }

    pBuilder->vertexCount = 0;
    pBuilder->indexCount = 0;
}