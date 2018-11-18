// Copyright (C) 2018 David Reid. See included LICENSE file.
//
// Credits to http://units.tauniverse.com/tutorials/tadesign/tadesign/ta-3do-fmtV2.txt for the description of the 3DO file format.

TA_PRIVATE taBool32 ta3DOInitObjectFromHeader(ta3DO* p3DO, ta3DOObject* pObject, ta3DOObjectHeader* pHeader)
{
    assert(pObject != NULL);
    memset(pObject, 0, sizeof(*pObject));

    pObject->header = *pHeader;
    pObject->name = p3DO->pFile->pFileData + pHeader->namePtr;

    //printf("OBJECT: %s\n", pObject->name);
    return TA_TRUE;
}

TA_PRIVATE ta3DOObject* ta3DOLoadObjectRecursive(ta3DO* p3DO)
{
    // PRE: The file must be sitting on the first byte of the object's header.

    assert(p3DO != NULL);

    ta3DOObjectHeader header;
    if (!ta3DOReadObjectHeader(p3DO->pFile, &header)) {
        return NULL;
    }

    ta3DOObject *pObject = (ta3DOObject*)malloc(sizeof(*pObject));
    if (pObject == NULL) {
        return NULL;
    }

    if (!ta3DOInitObjectFromHeader(p3DO, pObject, &header)) {
        free(pObject);
        return NULL;
    }

    // If the object has a sibling or child, they need to be loaded. The sibling comes first, but it doesn't really matter.

    // Next sibling.
    if (header.nextSiblingPtr != 0) {
        if (!taSeekFile(p3DO->pFile, header.nextSiblingPtr, taSeekOriginStart)) {
            free(pObject);
            return NULL;
        }

        pObject->pNextSibling = ta3DOLoadObjectRecursive(p3DO);
        if (pObject->pNextSibling == NULL) {
            free(pObject);
            return NULL;
        }
    }
    
    // First child.
    if (header.firstChildPtr != 0) {
        if (!taSeekFile(p3DO->pFile, header.firstChildPtr, taSeekOriginStart)) {
            free(pObject);
            return NULL;
        }

        pObject->pFirstChild = ta3DOLoadObjectRecursive(p3DO);
        if (pObject->pFirstChild == NULL) {
            free(pObject);
            return NULL;
        }
    }

    return pObject;
}


ta3DO* taOpen3DO(taFS* pFS, const char* fileName)
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

    ta3DO* p3DO = calloc(1, sizeof(*p3DO));
    if (p3DO == NULL) {
        goto on_error;
    }

    p3DO->pFile = taOpenFile(pFS, fullFileName, 0);
    if (p3DO->pFile == NULL) {
        goto on_error;
    }

    
    p3DO->pRootObject = ta3DOLoadObjectRecursive(p3DO);
    if (p3DO->pRootObject == NULL) {
        goto on_error;
    }


    return p3DO;

on_error:
    taClose3DO(p3DO);
    return NULL;
}

void taClose3DO(ta3DO* p3DO)
{
    if (p3DO == NULL) {
        return;
    }

    taCloseFile(p3DO->pFile);
    free(p3DO);
}

taBool32 ta3DOReadObjectHeader(taFile* pFile, ta3DOObjectHeader* pHeader)
{
    // PRE: The file must be sitting on the first byte of the header.

    assert(pFile != NULL);
    assert(pHeader != NULL);

    if (!taReadFileUInt32(pFile, &pHeader->version) || pHeader->version != 1 ||
        !taReadFileUInt32(pFile, &pHeader->vertexCount) ||
        !taReadFileUInt32(pFile, &pHeader->primitiveCount) ||
        !taReadFileUInt32(pFile, &pHeader->selectionPrimitivePtr) ||
        !taReadFileInt32( pFile, &pHeader->relativePosX) ||
        !taReadFileInt32( pFile, &pHeader->relativePosY) ||
        !taReadFileInt32( pFile, &pHeader->relativePosZ) ||
        !taReadFileUInt32(pFile, &pHeader->namePtr) ||
        !taReadFileUInt32(pFile, &pHeader->unused) ||
        !taReadFileUInt32(pFile, &pHeader->vertexPtr) ||
        !taReadFileUInt32(pFile, &pHeader->primitivePtr) ||
        !taReadFileUInt32(pFile, &pHeader->nextSiblingPtr) ||
        !taReadFileUInt32(pFile, &pHeader->firstChildPtr)) {
        return TA_FALSE;
    }

    return TA_TRUE;
}

taBool32 ta3DOReadPrimitiveHeader(taFile* pFile, ta3DOPrimitiveHeader* pHeaderOut)
{
    if (pFile == NULL || pHeaderOut == NULL) {
        return TA_FALSE;
    }

    if (!taReadFileUInt32(pFile, &pHeaderOut->colorIndex) ||
        !taReadFileUInt32(pFile, &pHeaderOut->indexCount) ||
        !taReadFileUInt32(pFile, &pHeaderOut->unused0) ||
        !taReadFileUInt32(pFile, &pHeaderOut->indexArrayPtr) ||
        !taReadFileUInt32(pFile, &pHeaderOut->textureNamePtr) ||
        !taReadFileUInt32(pFile, &pHeaderOut->unused1) ||
        !taReadFileUInt32(pFile, &pHeaderOut->unused2) ||
        !taReadFileUInt32(pFile, &pHeaderOut->isColored)) {
        return TA_FALSE;
    }

    return TA_TRUE;
}

taUInt32 ta3DOCountObjects(taFile* pFile)
{
    ta3DOObjectHeader header;
    if (!ta3DOReadObjectHeader(pFile, &header)) {
        return 0;
    }

    taUInt32 count = 1;

    // Siblings.
    if (header.nextSiblingPtr != 0) {
        if (!taSeekFile(pFile, header.nextSiblingPtr, taSeekOriginStart)) {
            return 0;
        }

        count += ta3DOCountObjects(pFile);
    }

    // Children.
    if (header.firstChildPtr != 0) {
        if (!taSeekFile(pFile, header.firstChildPtr, taSeekOriginStart)) {
            return 0;
        }

        count += ta3DOCountObjects(pFile);
    }


    return count;
}