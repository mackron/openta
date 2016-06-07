
bool ta_mesh_builder_init(ta_mesh_builder* pBuilder, size_t vertexSize)
{
    if (pBuilder == NULL || vertexSize == 0) {
        return false;
    }

    dr_zero_object(pBuilder);
    pBuilder->vertexSize = vertexSize;

    return true;
}

void ta_mesh_builder_uninit(ta_mesh_builder* pBuilder)
{
    if (pBuilder == NULL) {
        return;
    }

    free(pBuilder->pVertexData);
}


bool ta_mesh_builder_write_vertex(ta_mesh_builder* pBuilder, const void* pVertexData)
{
    if (pBuilder == NULL || pVertexData == NULL) {
        return false;
    }

    if (pBuilder->vertexCount == pBuilder->vertexBufferSize)
    {
        size_t newVertexBufferSize = (pBuilder->vertexBufferSize == 0) ? 128 : pBuilder->vertexBufferSize*2;
        void* pNewVertexData = realloc(pBuilder->pVertexData, newVertexBufferSize * pBuilder->vertexSize);
        if (pNewVertexData == NULL) {
            return false;
        }

        pBuilder->vertexBufferSize = newVertexBufferSize;
        pBuilder->pVertexData = pNewVertexData;
    }

    assert(pBuilder->vertexCount < pBuilder->vertexBufferSize);

    memcpy((uint8_t*)pBuilder->pVertexData + pBuilder->vertexCount*pBuilder->vertexSize, pVertexData, pBuilder->vertexSize);
    pBuilder->vertexCount += 1;
    return true;
}


void ta_mesh_builder_reset(ta_mesh_builder* pBuilder)
{
    if (pBuilder == NULL) {
        return;
    }

    pBuilder->vertexCount = 0;
}