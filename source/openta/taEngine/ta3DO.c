// Copyright (C) 2018 David Reid. See included LICENSE file.
//
// Credits to http://units.tauniverse.com/tutorials/tadesign/tadesign/ta-3do-fmtV2.txt for the description of the 3DO file format.

taBool32 ta_3do__init_object_from_header(ta_3do* p3DO, ta_3do_object* pObject, ta_3do_object_header* pHeader)
{
    assert(pObject != NULL);
    memset(pObject, 0, sizeof(*pObject));

    pObject->header = *pHeader;
    pObject->name = p3DO->pFile->pFileData + pHeader->namePtr;

    //printf("OBJECT: %s\n", pObject->name);
    return TA_TRUE;
}

ta_3do_object* ta_3do__load_object_recursive(ta_3do* p3DO)
{
    // PRE: The file must be sitting on the first byte of the object's header.

    assert(p3DO != NULL);

    ta_3do_object_header header;
    if (!ta_3do_read_object_header(p3DO->pFile, &header)) {
        return NULL;
    }

    ta_3do_object *pObject = (ta_3do_object*)malloc(sizeof(*pObject));
    if (pObject == NULL) {
        return NULL;
    }

    if (!ta_3do__init_object_from_header(p3DO, pObject, &header)) {
        free(pObject);
        return NULL;
    }

    // If the object has a sibling or child, they need to be loaded. The sibling comes first, but it doesn't really matter.

    // Next sibling.
    if (header.nextSiblingPtr != 0) {
        if (!ta_seek_file(p3DO->pFile, header.nextSiblingPtr, ta_seek_origin_start)) {
            free(pObject);
            return NULL;
        }

        pObject->pNextSibling = ta_3do__load_object_recursive(p3DO);
        if (pObject->pNextSibling == NULL) {
            free(pObject);
            return NULL;
        }
    }
    
    // First child.
    if (header.firstChildPtr != 0) {
        if (!ta_seek_file(p3DO->pFile, header.firstChildPtr, ta_seek_origin_start)) {
            free(pObject);
            return NULL;
        }

        pObject->pFirstChild = ta_3do__load_object_recursive(p3DO);
        if (pObject->pFirstChild == NULL) {
            free(pObject);
            return NULL;
        }
    }

    return pObject;
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

    
    p3DO->pRootObject = ta_3do__load_object_recursive(p3DO);
    if (p3DO->pRootObject == NULL) {
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

taBool32 ta_3do_read_object_header(ta_file* pFile, ta_3do_object_header* pHeader)
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
        !ta_read_file_uint32(pFile, &pHeader->firstChildPtr)) {
        return TA_FALSE;
    }

    return TA_TRUE;
}

taBool32 ta_3do_read_primitive_header(ta_file* pFile, ta_3do_primitive_header* pHeaderOut)
{
    if (pFile == NULL || pHeaderOut == NULL) {
        return TA_FALSE;
    }

    if (!ta_read_file_uint32(pFile, &pHeaderOut->colorIndex) ||
        !ta_read_file_uint32(pFile, &pHeaderOut->indexCount) ||
        !ta_read_file_uint32(pFile, &pHeaderOut->unused0) ||
        !ta_read_file_uint32(pFile, &pHeaderOut->indexArrayPtr) ||
        !ta_read_file_uint32(pFile, &pHeaderOut->textureNamePtr) ||
        !ta_read_file_uint32(pFile, &pHeaderOut->unused1) ||
        !ta_read_file_uint32(pFile, &pHeaderOut->unused2) ||
        !ta_read_file_uint32(pFile, &pHeaderOut->isColored)) {
        return TA_FALSE;
    }

    return TA_TRUE;
}

uint32_t ta_3do_count_objects(ta_file* pFile)
{
    ta_3do_object_header header;
    if (!ta_3do_read_object_header(pFile, &header)) {
        return 0;
    }

    uint32_t count = 1;

    // Siblings.
    if (header.nextSiblingPtr != 0) {
        if (!ta_seek_file(pFile, header.nextSiblingPtr, ta_seek_origin_start)) {
            return 0;
        }

        count += ta_3do_count_objects(pFile);
    }

    // Children.
    if (header.firstChildPtr != 0) {
        if (!ta_seek_file(pFile, header.firstChildPtr, ta_seek_origin_start)) {
            return 0;
        }

        count += ta_3do_count_objects(pFile);
    }


    return count;
}