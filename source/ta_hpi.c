// Credits to http://units.tauniverse.com/tutorials/tadesign/tadesign/ta-hpi-fmt.txt for explaining the HPI format.

// L7ZZ decompression.
bool ta_hpi__decompress_lz77(const unsigned char* pIn, uint32_t compressedSize, unsigned char* pOut, uint32_t uncompressedSize)
{
    const unsigned char* pInEnd = pIn + compressedSize;
    const unsigned char* pOutEnd = pOut + uncompressedSize;

    unsigned char block[4096];
    unsigned int iblock = 1;

    unsigned char tagMask = 1;
    unsigned char tag = *pIn++;

    while (pIn < pInEnd)
    {
        if ((tag & tagMask) == 0)
        {
            *pOut++       = *pIn;
            block[iblock] = *pIn++;

            iblock = (iblock + 1) & 0xFFF;
        }
        else
        {
            unsigned int offset = (pIn[1] << 4) | ((pIn[0] & 0xF0) >> 4);
            unsigned int length = (pIn[0] & 0x0F) + 2;

            if (offset == 0) {
                assert(pOut == pOutEnd);
                break;    // Done.
            }

            for (unsigned int i = 0; i < length; ++i)
            {
                *pOut++       = block[offset];
                block[iblock] = block[offset];

                iblock = (iblock + 1) & 0xFFF;
                offset = (offset + 1) & 0xFFF;
            }

            pIn += 2;
        }

        if (tagMask < 0x80) {
            tagMask <<= 1;
        } else {
            tagMask = 1;
            tag = *pIn++;
        }
    }

    return true;
}

// Zlib decompression.
bool ta_hpi__decompress_zlib(const void* pIn, uint32_t compressedSize, void* pOut, uint32_t uncompressedSize)
{
    // This is untested.
    return mz_uncompress(pOut, &uncompressedSize, pIn, compressedSize) == MZ_OK;
}


typedef struct
{
    // A pointer to the beginning of the buffer.
    const char* pData;

    // The size of the data.
    size_t dataSize;

    // The current read position.
    size_t currentReadPos;

} ta_hpi__memory_stream;


static size_t ta_hpi__read_archive(ta_hpi_archive* pHPI, void* pBufferOut, size_t bytesToRead)
{
    assert(pHPI != NULL);

    // For ease of use.
    uint8_t* pBufferOut8 = pBufferOut;

    // The data is encrypted which creates unnecessary inefficiency. Sigh.
    size_t bytesRead = pHPI->onRead(pHPI->pUserData, pBufferOut, bytesToRead);
    if (pHPI->header.key != 0) {
        ta_hpi_decrypt(pBufferOut, bytesRead, pHPI->decryptionKey, pHPI->currentPos);
    }
    

    pHPI->currentPos += bytesRead;
    return bytesRead;
}

static bool ta_hpi__read_archive_compressed(ta_hpi_archive* pHPI, void* pBufferOut, size_t uncompressedBytesToRead)
{
    size_t chunkCount = uncompressedBytesToRead / 65536;
    if ((uncompressedBytesToRead % 65536) > 0) {
        chunkCount += 1;
    }

    bool result = false;
    size_t* pChunkSizes = malloc(chunkCount * sizeof(*pChunkSizes));

    for (size_t iChunk = 0; iChunk < chunkCount; ++iChunk)
    {
        if (ta_hpi__read_archive(pHPI, &pChunkSizes[iChunk], 4) != 4) {
            result = false;
            goto finished;
        }
    }


    for (size_t iChunk = 0; iChunk < chunkCount; ++iChunk)
    {
        uint32_t marker = 0;
        if (ta_hpi__read_archive(pHPI, &marker, 4) != 4 || marker != 'HSQS') {
            result = false;
            goto finished;
        }

        uint8_t unused;
        if (ta_hpi__read_archive(pHPI, &unused, 1) != 1) {
            result = false;
            goto finished;
        }

        uint8_t compressionType;
        if (ta_hpi__read_archive(pHPI, &compressionType, 1) != 1) {
            result = false;
            goto finished;
        }

        uint8_t encrypted;
        if (ta_hpi__read_archive(pHPI, &encrypted, 1) != 1) {
            result = false;
            goto finished;
        }

        uint32_t compressedSize;
        if (ta_hpi__read_archive(pHPI, &compressedSize, 4) != 4) {
            result = false;
            goto finished;
        }

        uint32_t uncompressedSize;
        if (ta_hpi__read_archive(pHPI, &uncompressedSize, 4) != 4) {
            result = false;
            goto finished;
        }

        uint32_t checksum;
        if (ta_hpi__read_archive(pHPI, &checksum, 4) != 4) {
            result = false;
            goto finished;
        }


        unsigned char* pCompressedData = malloc(compressedSize);
        if (ta_hpi__read_archive(pHPI, pCompressedData, compressedSize) != compressedSize) {
            free(pCompressedData);
            result = false;
            goto finished;
        }

        
        // We do the decryption and checksum in one loop iteration.
        uint32_t checksum2 = 0;
        if (encrypted) {
            for (uint32_t i = 0; i < compressedSize; ++i) {
                checksum2 += pCompressedData[i];
                pCompressedData[i] = (pCompressedData[i] - i) ^ i;
            }
        } else {
            for (uint32_t i = 0; i < compressedSize; ++i) {
                checksum2 += pCompressedData[i];
            }
        }

        if (checksum != checksum2) {
            free(pCompressedData);
            result = false;
            goto finished;
        }


        // The actual decompression.
        switch (compressionType)
        {
            case 1: // LZ77
            {
                if (!ta_hpi__decompress_lz77(pCompressedData, compressedSize, (char*)pBufferOut + (iChunk * 65536), uncompressedSize)) {
                    free(pCompressedData);
                    result = false;
                    goto finished;
                }
            } break;

            case 2: // Zlib
            {
                if (!ta_hpi__decompress_zlib(pCompressedData, compressedSize, (char*)pBufferOut + (iChunk * 65536), uncompressedSize)) {
                    free(pCompressedData);
                    result = false;
                    goto finished;
                }
            } break;
        }


        free(pCompressedData);
    }

    result = true;


finished:
    free(pChunkSizes);
    return result;
}


static bool ta_hpi__seek_archive(ta_hpi_archive* pHPI, size_t offsetFromStart)
{
    assert(pHPI != NULL);

    if (!pHPI->onSeek(pHPI->pUserData, offsetFromStart)) {
        return false;
    }

    pHPI->currentPos = offsetFromStart;

    return true;
}

static size_t ta_hpi__read_from_memory(ta_hpi__memory_stream* pMemory, void* pDataOut, size_t bytesToRead)
{
    assert(pMemory != NULL);
    assert(pDataOut != NULL);
    assert(bytesToRead != 0);
    
    size_t bytesRemaining = pMemory->dataSize - pMemory->currentReadPos;
    if (bytesToRead > bytesRemaining) {
        bytesToRead = bytesRemaining;
    }

    if (bytesToRead > 0) {
        memcpy(pDataOut, pMemory->pData + pMemory->currentReadPos, bytesToRead);
        pMemory->currentReadPos += bytesToRead;
    }

    return bytesToRead;
}


bool ta_hpi__traverse_central_dir_subdir(ta_hpi__memory_stream* pCentralDirData, uint32_t centralDirPosInArchive, const char* parentDir, ta_hpi_central_dir_traversal_proc callback, void* pUserData)
{
    uint32_t fileCount;
    if (ta_hpi__read_from_memory(pCentralDirData, &fileCount, 4) != 4) {
        return false;
    }

    uint32_t entryOffset;
    if (ta_hpi__read_from_memory(pCentralDirData, &entryOffset, 4) != 4) {
        return false;
    }

    pCentralDirData->currentReadPos = entryOffset - centralDirPosInArchive;

    for (uint32_t i = 0; i < fileCount; ++i)
    {
        ta_hpi_central_dir_entry entry;
        if (ta_hpi__read_from_memory(pCentralDirData, &entry.namePos, 4) != 4) {
            return false;
        }
        if (ta_hpi__read_from_memory(pCentralDirData, &entry.dataPos, 4) != 4) {
            return false;
        }
        if (ta_hpi__read_from_memory(pCentralDirData, &entry.isDirectory, 1) != 1) {
            return false;
        }


        char filePath[TA_MAX_PATH];
        drpath_copy_and_append(filePath, sizeof(filePath), parentDir, (const char*)pCentralDirData->pData + (entry.namePos - centralDirPosInArchive));

        if (!callback(&entry, filePath, "", pUserData)) {
            return false;
        }


        if (entry.isDirectory)
        {
            // We need to save and restore the read position when doing a recursive call.
            uint32_t currentReadPos = pCentralDirData->currentReadPos;
            
            // Before recursively calling this function we need to move to the first byte of the subdir.
            pCentralDirData->currentReadPos = entry.dataPos - centralDirPosInArchive;
            ta_hpi__traverse_central_dir_subdir(pCentralDirData, centralDirPosInArchive, filePath, callback, pUserData);

            pCentralDirData->currentReadPos = currentReadPos;
        }
    }

    return true;
}

bool ta_hpi__traverse_central_dir(const void* pCentralDirData, size_t centralDirSizeInBytes, uint32_t centralDirPosInArchive, ta_hpi_central_dir_traversal_proc callback, void* pUserData)
{
    assert(pCentralDirData != NULL);
    assert(callback != NULL);

    ta_hpi__memory_stream centralDirStream;
    centralDirStream.pData = pCentralDirData;
    centralDirStream.dataSize = centralDirSizeInBytes;
    centralDirStream.currentReadPos = 0;
    return ta_hpi__traverse_central_dir_subdir(&centralDirStream, centralDirPosInArchive, "", callback, pUserData);
}



ta_hpi_archive* ta_open_hpi(ta_read_proc onRead, ta_seek_proc onSeek, void* pUserData)
{
    if (onRead == NULL || onSeek == NULL) {
        return NULL;
    }

    ta_hpi_header header;
    if (onRead(pUserData, &header, sizeof(header)) != sizeof(header)) {
        return NULL;
    }

    if (header.marker != 'IPAH') {
        return NULL;
    }

    // The directory size includes the header. This is not useful for us so normalize it.
    header.directorySize -= header.startPos;

    // Be sure to seek to the start of the real data.
    if (!onSeek(pUserData, header.startPos)) {
        return NULL;
    }


    // From this point on, everything is encrypted.
    uint32_t decryptionKey = ~((header.key * 4) | (header.key >> 6));
    if (header.key == 0) {
        decryptionKey = 0;
    }

    ta_hpi_archive* pHPI = malloc(sizeof(*pHPI) - sizeof(pHPI->pExtraData) + header.directorySize);
    if (pHPI == NULL) {
        return NULL;
    }

    pHPI->onRead        = onRead;
    pHPI->onSeek        = onSeek;
    pHPI->pUserData     = pUserData;
    pHPI->header        = header;
    pHPI->currentPos    = header.startPos;
    pHPI->decryptionKey = decryptionKey;

    // At this point everything is encrypted. The first chunk of data we want to read from here is the file listing.
    if (ta_hpi__read_archive(pHPI, pHPI->pExtraData, header.directorySize) != header.directorySize) {
        free(pHPI);
        return NULL;
    }

    return pHPI;
}

size_t ta_hpi__on_read_stdio(void* pUserData, void* bufferOut, size_t bytesToRead)
{
    return fread(bufferOut, 1, bytesToRead, (FILE*)pUserData);
}

bool ta_hpi__on_seek_stdio(void* pUserData, uint32_t offsetFromStart)
{
    return fseek((FILE*)pUserData, offsetFromStart, SEEK_SET) == 0;
}

ta_hpi_archive* ta_open_hpi_from_file(const char* filename)
{
    if (filename == NULL) {
        return NULL;
    }

    FILE* pFile;
#ifdef _MSC_VER
    if (fopen_s(&pFile, filename, "rb") != 0) {
        return NULL;
    }
#else
    pFile = fopen(filename, "rb");
    if (pFile == NULL) {
        return NULL;
    }
#endif

    return ta_open_hpi(ta_hpi__on_read_stdio, ta_hpi__on_seek_stdio, pFile);
}

void ta_close_hpi(ta_hpi_archive* pHPI)
{
    if (pHPI->onRead == ta_hpi__on_read_stdio) {
        fclose((FILE*)pHPI->pUserData);
    }
}


ta_hpi_file* ta_hpi_open_file(ta_hpi_archive* pHPI, const char* fileName)
{
    if (pHPI == NULL) {
        return NULL;
    }

    // The first thing we need to do is find the file.
    ta_hpi_ffi ffi;
    if (!ta_hpi_find_file(pHPI, fileName, &ffi)) {
        return NULL;
    }

    // We have found the file.
    if (ffi.entry.isDirectory) {
        return NULL;    // Don't want to try opening directories.
    }


    ta_hpi__memory_stream centralDirStream;
    centralDirStream.pData = pHPI->pExtraData;
    centralDirStream.dataSize = pHPI->header.directorySize;
    centralDirStream.currentReadPos = ffi.entry.dataPos - pHPI->header.startPos;

    uint32_t dataOffset;
    if (ta_hpi__read_from_memory(&centralDirStream, &dataOffset, 4) != 4) {
        return NULL;
    }

    uint32_t dataSize;
    if (ta_hpi__read_from_memory(&centralDirStream, &dataSize, 4) != 4) {
        return NULL;
    }

    uint8_t compressionType;
    if (ta_hpi__read_from_memory(&centralDirStream, &compressionType, 1) != 1) {
        return NULL;
    }


    // Seek to the first byte of the file within the archive.
    if (!ta_hpi__seek_archive(pHPI, dataOffset)) {
        return NULL;
    }


    ta_hpi_file* pFile = malloc(sizeof(*pFile) - sizeof(pFile->pFileData) + dataSize);
    if (pFile == NULL) {
        return NULL;
    }

    pFile->pHPI = pHPI;
    pFile->sizeInBytes = dataSize;
    pFile->readPointer = 0;

    // Here is where we need to read the file data. There is 3 types of compression used.
    //  0 - uncompressed
    //  1 - LZ77
    //  2 - Zlib
    if (compressionType == 0) {
        if (ta_hpi__read_archive(pHPI, pFile->pFileData, pFile->sizeInBytes) != pFile->sizeInBytes) {
            free(pFile);
            return NULL;
        }
    } else {
        if (!ta_hpi__read_archive_compressed(pHPI, pFile->pFileData, pFile->sizeInBytes)) {
            free(pFile);
            return NULL;
        }
    }

    return pFile;
}

void ta_hpi_close_file(ta_hpi_file* pFile)
{
    if (pFile == NULL) {
        return;
    }

    free(pFile);
}

bool ta_hpi_read(ta_hpi_file* pFile, void* pBufferOut, size_t bytesToRead, size_t* pBytesReadOut)
{
    if (pFile == NULL || pBufferOut == NULL) {
        return false;
    }

    size_t bytesAvailable = pFile->sizeInBytes - pFile->readPointer;
    if (bytesAvailable < bytesToRead) {
        bytesToRead = bytesAvailable;
    }

    if (bytesToRead == 0) {
        return false;   // Nothing left to read.
    }


    memcpy(pBufferOut, pFile->pFileData + pFile->readPointer, bytesToRead);
    pFile->readPointer += bytesToRead;

    if (pBytesReadOut) {
        *pBytesReadOut = bytesToRead;
    }

    return true;
}

bool ta_hpi_seek(ta_hpi_file* pFile, int64_t bytesToSeek, ta_seek_origin origin)
{
    uint64_t newPos = pFile->readPointer;
    if (origin == ta_seek_origin_current)
    {
        if ((int64_t)newPos + bytesToSeek >= 0) {
            newPos = (uint64_t)((int64_t)newPos + bytesToSeek);
        } else {
            // Trying to seek to before the beginning of the file.
            return false;
        }
    }
    else if (origin == ta_seek_origin_start)
    {
        assert(bytesToSeek >= 0);
        newPos = (uint64_t)bytesToSeek;
    }
    else if (origin == ta_seek_origin_end)
    {
        assert(bytesToSeek >= 0);
        if ((uint64_t)bytesToSeek <= pFile->sizeInBytes) {
            newPos = pFile->sizeInBytes - (uint64_t)bytesToSeek;
        } else {
            // Trying to seek to before the beginning of the file.
            return false;
        }
    }
    else
    {
        // Should never get here.
        return false;
    }


    if (newPos > pFile->sizeInBytes) {
        return false;
    }

    pFile->readPointer = (size_t)newPos;
    return true;
}

uint64_t ta_hpi_tell(ta_hpi_file* pFile)
{
    if (pFile == NULL) {
        return 0;
    }

    return pFile->readPointer;
}

uint64_t ta_hpi_size(ta_hpi_file* pFile)
{
    if (pFile == NULL) {
        return 0;
    }

    return pFile->sizeInBytes;
}



bool ta_hpi__find_file_traversal_callback(ta_hpi_central_dir_entry* pEntry, const char* filePath, const char* directoryPath, void* pUserData)
{
    (void)directoryPath;

    ta_hpi_ffi* ffi = pUserData;

    // It appears TA is not case sensitive.
    //if (strcmp(filePath, ffi->filePath) == 0) {
    if (_stricmp(filePath, ffi->filePath) == 0) {
        ffi->entry = *pEntry;
        ffi->exists = true;
        return false;   // <-- Stop traversing at this point.
    }

    return true;
}

bool ta_hpi_find_file(ta_hpi_archive* pHPI, const char* filePath, ta_hpi_ffi* ffiOut)
{
    ffiOut->filePath = filePath;
    ffiOut->exists = false;
    ta_hpi__traverse_central_dir(pHPI->pExtraData, pHPI->header.directorySize, pHPI->header.startPos, ta_hpi__find_file_traversal_callback, ffiOut);

    return ffiOut->exists;
}


bool ta_hpi_traverse_directory(ta_hpi_archive* pHPI, const char* directoryPath, bool recursive, ta_hpi_central_dir_traversal_proc callback, void* pUserData)
{
    ta_hpi_ffi ffi;
    if (!ta_hpi_find_file(pHPI, directoryPath, &ffi)) {
        return false;
    }

    ta_hpi__memory_stream centralDirStream;
    centralDirStream.pData = pHPI->pExtraData;
    centralDirStream.dataSize = pHPI->header.directorySize;
    centralDirStream.currentReadPos = ffi.entry.dataPos - pHPI->header.startPos;

    uint32_t fileCount;
    if (ta_hpi__read_from_memory(&centralDirStream, &fileCount, 4) != 4) {
        return false;
    }

    uint32_t entryOffset;
    if (ta_hpi__read_from_memory(&centralDirStream, &entryOffset, 4) != 4) {
        return false;
    }

    for (uint32_t i = 0; i < fileCount; ++i)
    {
        ta_hpi_central_dir_entry entry;
        if (ta_hpi__read_from_memory(&centralDirStream, &entry.namePos, 4) != 4) {
            return false;
        }
        if (ta_hpi__read_from_memory(&centralDirStream, &entry.dataPos, 4) != 4) {
            return false;
        }
        if (ta_hpi__read_from_memory(&centralDirStream, &entry.isDirectory, 1) != 1) {
            return false;
        }


        char filePath[TA_MAX_PATH];
        if (strncpy_s(filePath, sizeof(filePath), (const char*)centralDirStream.pData + (entry.namePos - pHPI->header.startPos), _TRUNCATE) != 0) {
            return false;
        }

        if (!callback(&entry, filePath, directoryPath, pUserData)) {
            return false;
        }

        if (recursive && entry.isDirectory) {
            char subdirPath[TA_MAX_PATH];
            if (!drpath_copy_and_append(subdirPath, sizeof(subdirPath), directoryPath, filePath)) {
                return false;
            }

            if (!ta_hpi_traverse_directory(pHPI, subdirPath, recursive, callback, pUserData)) {
                return false;
            }
        }
    }

    return true;
}


void* ta_hpi__open_and_read_file__generic(ta_hpi_archive* pHPI, const char* fileName, size_t* pSizeOut, size_t extraBytes)
{
    if (pSizeOut) {
        *pSizeOut = 0;
    }

    if (pHPI == NULL) {
        return NULL;
    }

    // The first thing we need to do is find the file.
    ta_hpi_ffi ffi;
    if (!ta_hpi_find_file(pHPI, fileName, &ffi)) {
        return NULL;
    }

    // We have found the file.
    if (ffi.entry.isDirectory) {
        return NULL;    // Don't want to try opening directories.
    }


    ta_hpi__memory_stream centralDirStream;
    centralDirStream.pData = pHPI->pExtraData;
    centralDirStream.dataSize = pHPI->header.directorySize;
    centralDirStream.currentReadPos = ffi.entry.dataPos - pHPI->header.startPos;

    uint32_t dataOffset;
    if (ta_hpi__read_from_memory(&centralDirStream, &dataOffset, 4) != 4) {
        return NULL;
    }

    uint32_t dataSize;
    if (ta_hpi__read_from_memory(&centralDirStream, &dataSize, 4) != 4) {
        return NULL;
    }

    if (dataSize > (SIZE_MAX - extraBytes)) {
        return NULL;    // File's too big.
    }


    uint8_t compressionType;
    if (ta_hpi__read_from_memory(&centralDirStream, &compressionType, 1) != 1) {
        return NULL;
    }


    // Seek to the first byte of the file within the archive.
    if (!ta_hpi__seek_archive(pHPI, dataOffset)) {
        return NULL;
    }


    void* pFileData = malloc(dataSize + extraBytes);
    if (pFileData == NULL) {
        return NULL;
    }

    // Here is where we need to read the file data. There is 3 types of compression used.
    //  0 - uncompressed
    //  1 - LZ77
    //  2 - Zlib
    if (compressionType == 0) {
        if (ta_hpi__read_archive(pHPI, pFileData, dataSize) != dataSize) {
            free(pFileData);
            return NULL;
        }
    } else {
        if (!ta_hpi__read_archive_compressed(pHPI, pFileData, dataSize)) {
            free(pFileData);
            return NULL;
        }
    }


    if (pSizeOut) {
        *pSizeOut = dataSize;
    }

    

    return pFileData;
}

void* ta_hpi_open_and_read_binary_file(ta_hpi_archive* pHPI, const char* fileName, size_t* pSizeOut)
{
    return ta_hpi__open_and_read_file__generic(pHPI, fileName, pSizeOut, 0);
}

char* ta_hpi_open_and_read_text_file(ta_hpi_archive* pHPI, const char* fileName, size_t* pLengthOut)
{
    char* pFileData = ta_hpi__open_and_read_file__generic(pHPI, fileName, pLengthOut, 1);   // <-- '1' means allocate 1 extra byte at the end. Used for the null terminator.
    if (pFileData == NULL) {
        return NULL;
    }

    // Just need to null terminate.
    pFileData[*pLengthOut] = '\0';
    return pFileData;
}
