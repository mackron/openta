// Public domain. See "unlicense" statement at the end of this file.
//
// Credits to http://units.tauniverse.com/tutorials/tadesign/tadesign/ta-3do-fmtV2.txt for the description of the 3DO file format.

typedef struct
{
    uint32_t version;
    uint32_t vertexCount;
    uint32_t primitiveCount;
    uint32_t selectionPrimitivePtr; // Only used by the root object.
    int32_t  relativePosX;
    int32_t  relativePosY;
    int32_t  relativePosZ;
    uint32_t namePtr;
    uint32_t unused;
    uint32_t vertexPtr;
    uint32_t primitivePtr;
    uint32_t nextSiblingPtr;
    uint32_t firstChildPtr;
} ta_3do_object_header;

bool ta_3do__read_object_header(ta_file* pFile, ta_3do_object_header* pHeader)
{
    // PRE: The file must be sitting on the first byte of the header.

    assert(pFile != NULL);
    assert(pHeader != NULL);

    if (!ta_read_file_uint32(pFile, &pHeader->version) || pHeader->version != 1 ||
        !ta_read_file_uint32(pFile, &pHeader->vertexCount) ||
        !ta_read_file_uint32(pFile, &pHeader->primitiveCount) ||
        !ta_read_file_uint32(pFile, &pHeader->selectionPrimitivePtr) ||
        !ta_read_file_int32( pFile, &pHeader->relativePosX) ||
        !ta_read_file_int32( pFile, &pHeader->relativePosY) ||
        !ta_read_file_int32( pFile, &pHeader->relativePosZ) ||
        !ta_read_file_uint32(pFile, &pHeader->namePtr) ||
        !ta_read_file_uint32(pFile, &pHeader->unused) ||
        !ta_read_file_uint32(pFile, &pHeader->vertexPtr) ||
        !ta_read_file_uint32(pFile, &pHeader->primitivePtr) ||
        !ta_read_file_uint32(pFile, &pHeader->nextSiblingPtr) ||
        !ta_read_file_uint32(pFile, &pHeader->firstChildPtr))
    {
        return false;
    }

    return true;
}


ta_3do* ta_open_3do(ta_fs* pFS, const char* fileName)
{
    if (pFS == NULL || fileName == NULL) {
        return NULL;
    }

    char fullFileName[TA_MAX_PATH];
    if (!drpath_copy_and_append(fullFileName, sizeof(fullFileName), "objects", fileName)) {
        return NULL;
    }
    if (!drpath_extension_equal(fileName, "3do")) {
        if (!drpath_append_extension(fullFileName, sizeof(fullFileName), "3do")) {
            return NULL;
        }
    }

    ta_3do* p3DO = calloc(1, sizeof(*p3DO));
    if (p3DO == NULL) {
        goto on_error;
    }

    p3DO->pFile = ta_open_file(pFS, fullFileName, 0);
    if (p3DO->pFile == NULL) {
        goto on_error;
    }

    
    ta_3do_object_header header;
    if (!ta_3do__read_object_header(p3DO->pFile, &header)) {
        goto on_error;
    }


    return p3DO;

on_error:
    ta_close_3do(p3DO);
    return NULL;
}

void ta_close_3do(ta_3do* p3DO)
{
    if (p3DO == NULL) {
        return;
    }

    ta_close_file(p3DO->pFile);
    free(p3DO);
}