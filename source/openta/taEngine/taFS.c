// Copyright (C) 2018 David Reid. See included LICENSE file.
//
// Credits to http://units.tauniverse.com/tutorials/tadesign/tadesign/ta-hpi-fmt.txt for details on the HPI format.

typedef struct
{
    taUInt32 marker;
    taUInt32 saveMarker;
    taUInt32 directorySize;
    taUInt32 key;
    taUInt32 startPos;
} taHPIHeader;

FILE* taFOpen(const char* filePath, const char* openMode)
{
    FILE* pFile;
#ifdef _MSC_VER
    if (fopen_s(&pFile, filePath, openMode) != 0) {
        return NULL;
    }
#else
    pFile = fopen(filePath, openMode);
    if (pFile == NULL) {
        return NULL;
    }
#endif

    return pFile;
}

int taFSeek(FILE* pFile, taInt64 offset, int origin)
{
#ifdef _WIN32
    return _fseeki64(pFile, offset, origin);
#else
    return fseek64(pFile, offset, origin);
#endif
}

taInt64 taFTell(FILE* pFile)
{
#ifdef _WIN32
    return _ftelli64(pFile);
#else
    return ftell64(pFile);
#endif
}


TA_PRIVATE taBool32 taFSFindFileInArchive(taFS* pFS, taFSArchive* pArchive, const char* fileRelativePath, taUInt32* pDataPosOut)
{
    assert(pFS != NULL);
    assert(pArchive != NULL);
    assert(fileRelativePath != NULL);
    assert(pDataPosOut != NULL);

    taMemoryStream stream = taCreateMemoryStream(pArchive->pCentralDirectory, pArchive->centralDirectorySize);

    // Finding the file within the archive is simple - we just search by each path segment in order and keep going until we
    // either find the file or don't find a segment.
    drpath_iterator pathseg;
    if (!drpath_first(fileRelativePath, &pathseg)) {
        return TA_FALSE;
    }

    do {
        // At this point we will be sitting on the first byte of the sub-directory.
        taUInt32 fileCount;
        if (taMemoryStreamRead(&stream, &fileCount, 4) != 4) {
            return TA_FALSE;
        }

        taUInt32 entryOffset;
        if (taMemoryStreamRead(&stream, &entryOffset, 4) != 4) {
            return TA_FALSE;
        }

        taBool32 subdirExists = TA_FALSE;
        for (taUInt32 iFile = 0; iFile < fileCount; ++iFile) {
            taUInt32 namePos;
            if (taMemoryStreamRead(&stream, &namePos, 4) != 4) {
                return TA_FALSE;
            }

            taUInt32 dataPos;
            if (taMemoryStreamRead(&stream, &dataPos, 4) != 4) {
                return TA_FALSE;
            }

            taUInt8 isDirectory;
            if (taMemoryStreamRead(&stream, &isDirectory, 1) != 1) {
                return TA_FALSE;
            }


            if (_strnicmp(pArchive->pCentralDirectory + namePos, pathseg.path + pathseg.segment.offset, pathseg.segment.length) == 0) {
                subdirExists = TA_TRUE;

                // If it's not a directory, but we've still got segments left in the path then there's an error.
                if (pathseg.path[pathseg.segment.offset + pathseg.segment.length] == '\0') {
                    // It's the last segment - this is the file we're after.
                    *pDataPosOut = dataPos;
                    return TA_TRUE;
                } else {
                    if (!isDirectory) {
                        return TA_FALSE;   // We have path segments remaining, but we found a file (not a folder) in the listing. This is erroneous.
                    }

                    // Before continuing we need to move the streamer to the start of the sub-directory.
                    if (!taMemoryStreamSeek(&stream, dataPos, taSeekOriginStart)) {
                        return TA_FALSE;
                    }
                }

                break;
            }
        }

        if (!subdirExists) {
            return TA_FALSE;
        }
    } while (drpath_next(&pathseg));

    return TA_FALSE;
}


TA_PRIVATE taBool32 taFSFileExistsInList(const char* relativePath, taUInt32 fileCount, taFSFileInfo* pFiles)
{
    if (pFiles == NULL) {
        return TA_FALSE;
    }

    for (taUInt32 i = 0; i < fileCount; ++i) {
        if (_stricmp(pFiles[i].relativePath, relativePath) == 0) {    // <-- TA seems to be case insensitive.
            return TA_TRUE;
        }
    }

    return TA_FALSE;
}

TA_PRIVATE void taFSGatherFilesInNativeDirectory(taFS* pFS, const char* directoryRelativePath, taBool32 recursive, taUInt32* pFileCountInOut, taFSFileInfo** ppFilesInOut)
{
    assert(pFS != NULL);
    assert(directoryRelativePath != NULL);
    assert(pFileCountInOut != NULL);
    assert(ppFilesInOut != NULL);

#ifdef _WIN32
    char searchQuery[TA_MAX_PATH];
    if (!drpath_copy_and_append(searchQuery, sizeof(searchQuery), pFS->rootDir, directoryRelativePath)) {
        return;
    }

    unsigned int searchQueryLength = (unsigned int)strlen(searchQuery);
    if (searchQueryLength >= TA_MAX_PATH - 3) {
        return;    // Path is too long.
    }

    searchQuery[searchQueryLength + 0] = '\\';
    searchQuery[searchQueryLength + 1] = '*';
    searchQuery[searchQueryLength + 2] = '\0';

    taUInt32 fileCount = 0;

    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(searchQuery, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return; // Failed to begin search.
    }

    do
    {
        // Skip past "." and ".." directories.
        if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0) {
            continue;
        }

        taFSFileInfo fi;
        fi.archiveRelativePath[0] = '\0';
        drpath_copy_and_append(fi.relativePath, sizeof(fi.relativePath), directoryRelativePath, ffd.cFileName);
        fi.isDirectory = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        // Skip past the file if it's already in the list.
        if (taFSFileExistsInList(fi.relativePath, *pFileCountInOut, *ppFilesInOut)) {
            continue;
        }



        taFSFileInfo* pNewFiles = realloc(*ppFilesInOut, ((*pFileCountInOut) + fileCount + 1) * sizeof(**ppFilesInOut));
        if (pNewFiles == NULL) {
            break;
        }

        (*ppFilesInOut) = pNewFiles;
        (*ppFilesInOut)[*pFileCountInOut + fileCount] = fi;
        fileCount += 1;


        if (recursive && ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            taFSGatherFilesInNativeDirectory(pFS, fi.relativePath, recursive, pFileCountInOut, ppFilesInOut);
        }

    } while (FindNextFileA(hFind, &ffd));

    FindClose(hFind);
    *pFileCountInOut += fileCount;
#else
    // TODO: Posix
#endif
}

TA_PRIVATE void taFSGatherFilesInArchiveDirectoryAtLocation(taFS* pFS, taFSArchive* pArchive, taUInt32 directoryDataPos, const char* parentPath, taBool32 recursive, taUInt32* pFileCountInOut, taFSFileInfo** ppFilesInOut, taUInt32 prevFileCount)
{
    taMemoryStream stream = taCreateMemoryStream(pArchive->pCentralDirectory, pArchive->centralDirectorySize);
    if (!taMemoryStreamSeek(&stream, directoryDataPos, taSeekOriginStart)) {
        return;
    }

    taUInt32 fileCount;
    if (taMemoryStreamRead(&stream, &fileCount, 4) != 4) {
        return;
    }

    taUInt32 entryOffset;
    if (taMemoryStreamRead(&stream, &entryOffset, 4) != 4) {
        return;
    }

    for (taUInt32 iFile = 0; iFile < fileCount; ++iFile) {
        taUInt32 namePos;
        if (taMemoryStreamRead(&stream, &namePos, 4) != 4) {
            return;
        }

        taUInt32 dataPos;
        if (taMemoryStreamRead(&stream, &dataPos, 4) != 4) {
            return;
        }

        taUInt8 isDirectory;
        if (taMemoryStreamRead(&stream, &isDirectory, 1) != 1) {
            return;
        }


        taFSFileInfo fi;
        strncpy_s(fi.archiveRelativePath, sizeof(fi.archiveRelativePath), pArchive->relativePath, _TRUNCATE);
        drpath_copy_and_append(fi.relativePath, sizeof(fi.relativePath), parentPath, pArchive->pCentralDirectory + namePos);
        fi.isDirectory = isDirectory;

        // Skip past the file if it's already in the list. Never skip directories.
        if (!fi.isDirectory && taFSFileExistsInList(fi.relativePath, prevFileCount, *ppFilesInOut)) {       // <-- use prevFileCount here to make this search more efficient.
            continue;
        }


        taFSFileInfo* pNewFiles = realloc(*ppFilesInOut, ((*pFileCountInOut) + 1) * sizeof(**ppFilesInOut));
        if (pNewFiles == NULL) {
            return;
        }

        (*ppFilesInOut) = pNewFiles;
        (*ppFilesInOut)[*pFileCountInOut] = fi;
        *pFileCountInOut += 1;


        if (recursive && isDirectory) {
            taFSGatherFilesInArchiveDirectoryAtLocation(pFS, pArchive, dataPos, fi.relativePath, recursive, pFileCountInOut, ppFilesInOut, prevFileCount);
        }
    }
}

TA_PRIVATE void taFSGatherFilesInArchiveDirectory(taFS* pFS, taFSArchive* pArchive, const char* directoryRelativePath, taBool32 recursive, taUInt32* pFileCountInOut, taFSFileInfo** ppFilesInOut)
{
    assert(pFS != NULL);
    assert(directoryRelativePath != NULL);
    assert(pFileCountInOut != NULL);
    assert(ppFilesInOut != NULL);

    // The first thing to do is find the entry within the central directory that represents the directory. 
    taUInt32 directoryDataPos;
    if (!taFSFindFileInArchive(pFS, pArchive, directoryRelativePath, &directoryDataPos)) {
        return;
    }

    // We found the directory, so now we need to gather the files within it.
    taFSGatherFilesInArchiveDirectoryAtLocation(pFS, pArchive, directoryDataPos, directoryRelativePath, recursive, pFileCountInOut, ppFilesInOut, *pFileCountInOut);
}

TA_PRIVATE int taFSFileInfoQuickSortCallback(const void* a, const void* b)
{
    const taFSFileInfo* pInfoA = a;
    const taFSFileInfo* pInfoB = b;

    return strcmp(pInfoA->relativePath, pInfoB->relativePath);
}


TA_PRIVATE taBool32 taFSAdjustCentralDirectoryNamePointersRecursive(taMemoryStream* pStream, taUInt32 negativeOffset)
{
    assert(pStream != NULL);

    taUInt32 fileCount;
    if (taMemoryStreamRead(pStream, &fileCount, 4) != 4) {
        return TA_FALSE;
    }

    taUInt32 entryOffset;
    if (taMemoryStreamPeek(pStream, &entryOffset, 4) != 4) {     // <-- Note that it's a peek and not a read. Reason is because this value is going to be replaced.
        return TA_FALSE;
    }

    // The entry offset is within the central directory. Need to adjust this pointer by simply subtracting negativeOffset.
    assert(entryOffset >= negativeOffset);
    entryOffset -= negativeOffset;
    taMemoryStreamWriteUInt32(pStream, entryOffset);

    // Now we need to seek to each file and do the same offsetting for them.
    if (!taMemoryStreamSeek(pStream, entryOffset, taSeekOriginStart)) {
        return TA_FALSE;
    }

    for (taUInt32 i = 0; i < fileCount; ++i) {
        taUInt32 namePos;
        if (taMemoryStreamPeek(pStream, &namePos, 4) != 4) {     // <-- Note that it's a peek and not a read. Reason is because this value is going to be replaced.
            return TA_FALSE;
        }

        assert(namePos >= negativeOffset);
        namePos -= negativeOffset;
        taMemoryStreamWriteUInt32(pStream, namePos);


        taUInt32 dataPos;
        if (taMemoryStreamPeek(pStream, &dataPos, 4) != 4) {     // <-- Note that it's a peek and not a read. Reason is because this value is going to be replaced.
            return TA_FALSE;
        }

        assert(dataPos >= negativeOffset);
        dataPos -= negativeOffset;
        taMemoryStreamWriteUInt32(pStream, dataPos);


        taUInt8 isDirectory;
        if (taMemoryStreamRead(pStream, &isDirectory, 1) != 1) {
            return TA_FALSE;
        }

        // If it's a directory we need to call this recursively. The current read position of the stream needs to be saved and restored in order for
        // the recursion to work correctly.
        if (isDirectory) {
            taUInt32 currentReadPos = (taUInt32)taMemoryStreamTell(pStream);

            // Before traversing the sub-directory we need to seek to it.
            if (!taMemoryStreamSeek(pStream, dataPos, taSeekOriginStart)) {
                return TA_FALSE;
            }

            if (!taFSAdjustCentralDirectoryNamePointersRecursive(pStream, negativeOffset)) {
                return TA_FALSE;
            }

            
            // We're done with the sub-directory, but traversing that will have changed the read position of the memory stream, so that will need
            // to be restored before continuing.
            if (!taMemoryStreamSeek(pStream, currentReadPos, taSeekOriginStart)) {
                return TA_FALSE;
            }
        }
    }

    return TA_TRUE;
}

TA_PRIVATE taBool32 taFSAdjustCentralDirectoryNamePointers(char* pCentralDirectory, taUInt32 centralDirectorySize, taUInt32 negativeOffset)
{
    assert(pCentralDirectory != NULL);
    assert(negativeOffset > 0);

    taMemoryStream stream;
    stream.pData = pCentralDirectory;
    stream.dataSize = centralDirectorySize;
    stream.currentReadPos = 0;
    return taFSAdjustCentralDirectoryNamePointersRecursive(&stream, negativeOffset);
}

TA_PRIVATE taBool32 taFSRegisterArchive(taFS* pFS, const char* archiveRelativePath)
{
    assert(pFS != NULL);
    assert(archiveRelativePath != NULL);

    // If an archive of the same path already exists, ignore it.
    for (size_t iArchive = 0; iArchive < pFS->archiveCount; ++iArchive) {
        if (_stricmp(pFS->pArchives[iArchive].relativePath, archiveRelativePath) == 0) {    // <-- TA seems to be case insensitive.
            return TA_TRUE;
        }
    }


    // Before registering the archive we want to ensure it's valid. We just read the header for this.
    char archiveAbsolutePath[TA_MAX_PATH];
    if (!drpath_copy_and_append(archiveAbsolutePath, sizeof(archiveAbsolutePath), pFS->rootDir, archiveRelativePath)) {
        return TA_FALSE;
    }

    FILE* pFile = taFOpen(archiveAbsolutePath, "rb");
    if (pFile == NULL) {
        return TA_FALSE;
    }


    // Header
    taHPIHeader header;
    if (fread(&header, 1, sizeof(header), pFile) != sizeof(header)) {
        fclose(pFile);
        return TA_FALSE;
    }

    if (header.marker != 'IPAH') {
        fclose(pFile);
        return TA_FALSE;    // Not a HPI file.
    }


    // It appears to be a valid archive - add it to the list.
    taFSArchive* pNewArchives = realloc(pFS->pArchives, (pFS->archiveCount + 1) * sizeof(*pNewArchives));
    if (pNewArchives == NULL) {
        fclose(pFile);
        return TA_FALSE;
    }

    pFS->pArchives = pNewArchives;


    taFSArchive* pArchive = pNewArchives + pFS->archiveCount;
    if (strncpy_s(pArchive->relativePath, sizeof(pArchive->relativePath), archiveRelativePath, _TRUNCATE) != 0) {
        fclose(pFile);
        return TA_FALSE;
    }

    if (header.key == 0) {
        pArchive->decryptionKey = 0;
    } else {
        pArchive->decryptionKey = ~((header.key * 4) | (header.key >> 6));
    }

    // Central directory. A few things here: 1) it starts at header.startPos; 2) it's encrypted; 3) the name pointers need to be adjusted
    // so that they're relative to the start of the central directory data.
    pArchive->centralDirectorySize = header.directorySize - header.startPos;    // <-- Subtract header.startPos because the directory size includes the size of the header.

    if (taFSeek(pFile, header.startPos, SEEK_SET) != 0) {
        fclose(pFile);
        return TA_FALSE;
    }

    pArchive->pCentralDirectory = malloc(pArchive->centralDirectorySize);
    if (pArchive->pCentralDirectory == NULL) {
        fclose(pFile);
        return TA_FALSE;
    }

    if (fread(pArchive->pCentralDirectory, 1, pArchive->centralDirectorySize, pFile) != pArchive->centralDirectorySize) {
        free(pArchive->pCentralDirectory);
        fclose(pFile);
        return TA_FALSE;
    }
    
    fclose(pFile);


    // The central directory needs to be decrypted.
    taHPIDecrypt(pArchive->pCentralDirectory, pArchive->centralDirectorySize, pArchive->decryptionKey, header.startPos);

    // Now we need to recursively traverse the central directory and adjust the pointers to the file names so that they're
    // relative to the central directory. To do this we just offset it by -header.startPos. By doing this now we can simplify
    // future traversals.
    if (!taFSAdjustCentralDirectoryNamePointers(pArchive->pCentralDirectory, pArchive->centralDirectorySize, header.startPos)) {
        free(pArchive->pCentralDirectory);
        return TA_FALSE;
    }

    
    pFS->archiveCount += 1;
    return TA_TRUE;
}


TA_PRIVATE taFile* taFSOpenFileFromArchive(taFS* pFS, taFSArchive* pArchive, const char* fileRelativePath, unsigned int options)
{
    taUInt32 dataPos;
    if (!taFSFindFileInArchive(pFS, pArchive, fileRelativePath, &dataPos)) {
        return NULL;
    }

    // It's in this archive.
    char archiveAbsolutePath[TA_MAX_PATH];
    if (!drpath_copy_and_append(archiveAbsolutePath, sizeof(archiveAbsolutePath), pFS->rootDir, pArchive->relativePath)) {
        return NULL;
    }

    FILE* pSTDIOFile = taFOpen(archiveAbsolutePath, "rb");
    if (pSTDIOFile == NULL) {
        return NULL;
    }


    taMemoryStream stream = taCreateMemoryStream(pArchive->pCentralDirectory, pArchive->centralDirectorySize);
    taMemoryStreamSeek(&stream, dataPos, taSeekOriginStart);

    taUInt32 dataOffset;
    if (taMemoryStreamRead(&stream, &dataOffset, 4) != 4) {
        return NULL;
    }

    taUInt32 dataSize;
    if (taMemoryStreamRead(&stream, &dataSize, 4) != 4) {
        return NULL;
    }

    taUInt8 compressionType;
    if (taMemoryStreamRead(&stream, &compressionType, 1) != 1) {
        return NULL;
    }


    // Seek to the first byte of the file within the archive.
    if (taFSeek(pSTDIOFile, dataOffset, taSeekOriginStart) != 0) {
        return NULL;
    }


    taUInt32 extraBytes = 0;
    if (options & TA_OPEN_FILE_WITH_NULL_TERMINATOR) {
        extraBytes = 1;
    }
    

    taFile* pFile = malloc(sizeof(*pFile) + dataSize + extraBytes);
    if (pFile == NULL) {
        fclose(pSTDIOFile);
        return NULL;
    }

    pFile->_stream = taCreateMemoryStream(pFile->pFileData, dataSize);
    pFile->pFS = pFS;
    pFile->sizeInBytes = dataSize;

    // Here is where we need to read the file data. There is 3 types of compression used.
    //  0 - uncompressed
    //  1 - LZ77
    //  2 - Zlib
    if (compressionType == 0) {
        if (taHPIReadAndDecrypt(pSTDIOFile, pFile->pFileData, pFile->sizeInBytes, pArchive->decryptionKey) != pFile->sizeInBytes) {
            fclose(pSTDIOFile);
            free(pFile);
            return NULL;
        }
    } else {
        if (!taHPIReadAndDecryptCompressed(pSTDIOFile, pFile->pFileData, pFile->sizeInBytes, pArchive->decryptionKey)) {
            fclose(pSTDIOFile);
            free(pFile);
            return NULL;
        }
    }

    // Null terminate if required.
    if (options & TA_OPEN_FILE_WITH_NULL_TERMINATOR) {
        pFile->pFileData[pFile->sizeInBytes] = '\0';
    }


    fclose(pSTDIOFile);
    return pFile;
}


taFS* taCreateFileSystem()
{
    // We need to retrieve the root directory first. We can't continue if this fails.
    char exedir[TA_MAX_PATH];
    if (!dr_get_executable_path(exedir, sizeof(exedir))) {
        return NULL;
    }
    drpath_remove_file_name(exedir);



    taFS* pFS = calloc(1, sizeof(*pFS));
    if (pFS == NULL) {
        return NULL;
    }

    if (strcpy_s(pFS->rootDir, sizeof(pFS->rootDir), exedir) != 0) {
        taDeleteFileSystem(pFS);
        return NULL;
    }

    pFS->archiveCount = 0;
    pFS->pArchives = NULL;

    // Now we want to try loading all of the known archive files. There are a few mandatory packages which if not present will result
    // in this failing to initialize.
    if (!taFSRegisterArchive(pFS, "rev31.gp3")   ||
        !taFSRegisterArchive(pFS, "totala1.hpi") ||
        !taFSRegisterArchive(pFS, "totala2.hpi"))
    {
        taDeleteFileSystem(pFS);
        return TA_FALSE;
    }

    // Optional packages.
    taFSRegisterArchive(pFS, "totala4.hpi");
    taFSRegisterArchive(pFS, "ccdata.ccx");
    taFSRegisterArchive(pFS, "ccmaps.ccx");
    taFSRegisterArchive(pFS, "ccmiss.ccx");
    taFSRegisterArchive(pFS, "btdata.ccx");
    taFSRegisterArchive(pFS, "btmaps.ccx");
    taFSRegisterArchive(pFS, "tactics1.hpi");
    taFSRegisterArchive(pFS, "tactics2.hpi");
    taFSRegisterArchive(pFS, "tactics3.hpi");
    taFSRegisterArchive(pFS, "tactics4.hpi");
    taFSRegisterArchive(pFS, "tactics5.hpi");
    taFSRegisterArchive(pFS, "tactics6.hpi");
    taFSRegisterArchive(pFS, "tactics7.hpi");
    taFSRegisterArchive(pFS, "tactics8.hpi");

    // Now we need to search for .ufo files and register them. We only search the root directory for these.
    taUInt32 fileCount = 0;
    taFSFileInfo* pFiles = NULL;
    taFSGatherFilesInNativeDirectory(pFS, "", TA_FALSE, &fileCount, &pFiles);
    qsort(pFiles, fileCount, sizeof(*pFiles), taFSFileInfoQuickSortCallback);

    for (taUInt32 iFile = 0; iFile < fileCount; ++iFile) {
        if (!pFiles[iFile].isDirectory && drpath_extension_equal(pFiles[iFile].relativePath, "ufo")) {
            taFSRegisterArchive(pFS, pFiles[iFile].relativePath);
        }
    }
    

    free(pFiles);
    return pFS;
}

void taDeleteFileSystem(taFS* pFS)
{
    if (pFS == NULL) {
        return;
    }

    if (pFS->pArchives != NULL) {
        for (taUInt32 iArchive = 0; iArchive < pFS->archiveCount; ++iArchive) {
            free(pFS->pArchives[iArchive].pCentralDirectory);
        }

        free(pFS->pArchives);
    }

    
    free(pFS);
}


taFile* taOpenSpecificFile(taFS* pFS, const char* archiveRelativePath, const char* fileRelativePath, unsigned int options)
{
    if (pFS == NULL) {
        return NULL;
    }


    if (archiveRelativePath == NULL || archiveRelativePath[0] == '\0') {
        // No archive file was specified. Try opening from the native file system.
        char fileAbsolutePath[TA_MAX_PATH];
        if (!drpath_copy_and_append(fileAbsolutePath, sizeof(fileAbsolutePath), pFS->rootDir, fileRelativePath)) {
            return NULL;
        }

        FILE* pSTDIOFile = taFOpen(fileAbsolutePath, "rb");
        if (pSTDIOFile == NULL) {
            return NULL;
        }

        taUInt32 extraBytes = 0;
        if (options & TA_OPEN_FILE_WITH_NULL_TERMINATOR) {
            extraBytes = 1;
        }


        taFSeek(pSTDIOFile, 0, SEEK_END);
        taUInt64 sizeInBytes = taFTell(pSTDIOFile);
        taFSeek(pSTDIOFile, 0, SEEK_SET);

        if (sizeInBytes > (sizeInBytes - extraBytes)) {
            fclose(pSTDIOFile);
            return NULL;    // File's too big.
        }


        taFile* pFile = malloc(sizeof(*pFile) + (size_t)sizeInBytes + extraBytes);
        if (pFile == NULL) {
            fclose(pSTDIOFile);
            return NULL;
        }

        pFile->_stream = taCreateMemoryStream(pFile->pFileData, (size_t)sizeInBytes);
        pFile->pFS = pFS;
        pFile->sizeInBytes = (size_t)sizeInBytes;

        if (!fread(pFile->pFileData, 1, pFile->sizeInBytes, pSTDIOFile)) {
            fclose(pSTDIOFile);
            return NULL;
        }


        // Null terminate if required.
        if (options & TA_OPEN_FILE_WITH_NULL_TERMINATOR) {
            pFile->pFileData[pFile->sizeInBytes] = '\0';
        }

        fclose(pSTDIOFile);
        return pFile;
    } else {
        // An archive file was specified. Try opening from that archive. To do this we need to search the central directory.
        for (taUInt32 iArchive = 0; iArchive < pFS->archiveCount; ++iArchive) {
            if (_stricmp(pFS->pArchives[iArchive].relativePath, archiveRelativePath) == 0) {
                return taFSOpenFileFromArchive(pFS, &pFS->pArchives[iArchive], fileRelativePath, options);
            }
        }

        return NULL;
    }
}

taFile* taOpenFile(taFS* pFS, const char* relativePath, unsigned int options)
{
    // All we need to do is try opening the file from every archive, in order of priority.

    // The native file system is highest priority.
    taFile* pFile = taOpenSpecificFile(pFS, NULL, relativePath, options);
    if (pFile != NULL) {
        return pFile;   // It's on the native file system.
    }


    // At this point the file is not on the native file system so we just need to search for it.
    for (taUInt32 iArchive = 0; iArchive < pFS->archiveCount; ++iArchive) {
        pFile = taFSOpenFileFromArchive(pFS, &pFS->pArchives[iArchive], relativePath, options);
        if (pFile != NULL) {
            return pFile;
        }
    }


    // Didn't find the file...
    assert(pFile == NULL);
    return NULL;
}

void taCloseFile(taFile* pFile)
{
    if (pFile == NULL) {
        return;
    }

    free(pFile);
}


taBool32 taReadFile(taFile* pFile, void* pBufferOut, size_t bytesToRead, size_t* pBytesReadOut)
{
    if (pFile == NULL || pBufferOut == NULL) {
        return TA_FALSE;
    }

    size_t bytesRead = taMemoryStreamRead(&pFile->_stream, pBufferOut, bytesToRead);
    if (pBytesReadOut) {
        *pBytesReadOut = bytesRead;
    }

    return TA_TRUE;
}

taBool32 taSeekFile(taFile* pFile, taInt64 bytesToSeek, taSeekOrigin origin)
{
    if (pFile == NULL) {
        return TA_FALSE;
    }

    return taMemoryStreamSeek(&pFile->_stream, bytesToSeek, origin);
}

taUInt64 taTellFile(taFile* pFile)
{
    if (pFile == NULL) {
        return TA_FALSE;
    }

    return (taUInt64)taMemoryStreamTell(&pFile->_stream);
}


taBool32 taReadFileUInt32(taFile* pFile, taUInt32* pBufferOut)
{
    size_t bytesRead;
    return taReadFile(pFile, pBufferOut, sizeof(taUInt32), &bytesRead) && bytesRead == sizeof(taUInt32);
}

taBool32 taReadFileInt32(taFile* pFile, taInt32* pBufferOut)
{
    size_t bytesRead;
    return taReadFile(pFile, pBufferOut, sizeof(taInt32), &bytesRead) && bytesRead == sizeof(taInt32);
}

taBool32 taReadFileUInt16(taFile* pFile, taUInt16* pBufferOut)
{
    size_t bytesRead;
    return taReadFile(pFile, pBufferOut, sizeof(taUInt16), &bytesRead) && bytesRead == sizeof(taUInt16);
}

taBool32 taReadFileUInt8(taFile* pFile, taUInt8* pBufferOut)
{
    size_t bytesRead;
    return taReadFile(pFile, pBufferOut, sizeof(taUInt8), &bytesRead) && bytesRead == sizeof(taUInt8);
}



//// Iteration ////

TA_PRIVATE taUInt32 taFSGatherFilesInDirectory(taFS* pFS, const char* directoryRelativePath, taFSFileInfo** ppFilesInOut, taBool32 recursive)
{
    assert(pFS != NULL);
    assert(directoryRelativePath != NULL);
    assert(ppFilesInOut != NULL);

    taUInt32 fileCount = 0;

    // Native directory has the highest priority.
    taFSGatherFilesInNativeDirectory(pFS, directoryRelativePath, recursive, &fileCount, ppFilesInOut);
    
    // Archives after the native directory.
    for (taUInt32 iArchive = 0; iArchive < pFS->archiveCount; ++iArchive) {
        taFSGatherFilesInArchiveDirectory(pFS, &pFS->pArchives[iArchive], directoryRelativePath, recursive, &fileCount, ppFilesInOut);
    }

    return fileCount;
}

taFSIterator* taFSBegin(taFS* pFS, const char* directoryRelativePath, taBool32 recursive)
{
    if (pFS == NULL) {
        return NULL;
    }

    // If the directory path is null, just treat it as though we are iteration over the root directory.
    if (directoryRelativePath == NULL) {
        directoryRelativePath = "";
    }

    taFSIterator* pIter = calloc(1, sizeof(*pIter));
    if (pIter == NULL) {
        return NULL;
    }

    pIter->pFS = pFS;
    pIter->_fileCount = taFSGatherFilesInDirectory(pFS, directoryRelativePath, &pIter->_pFiles, recursive);

    return pIter;
}

void taFSEnd(taFSIterator* pIter)
{
    if (pIter == NULL) {
        return;
    }

    free(pIter->_pFiles);
    free(pIter);
}

taBool32 taFSNext(taFSIterator* pIter)
{
    if (pIter == NULL || pIter->_iFile >= pIter->_fileCount) {
        return TA_FALSE;
    }

    assert(pIter->_pFiles != NULL);

    pIter->fileInfo = pIter->_pFiles[pIter->_iFile];
    pIter->_iFile += 1;

    return TA_TRUE;
}



//// HPI Helpers ////

taBool32 taHPIDecompressLZ77(const unsigned char* pIn, taUInt32 compressedSize, unsigned char* pOut, taUInt32 uncompressedSize)
{
    const unsigned char* pInEnd = pIn + compressedSize;
    const unsigned char* pOutEnd = pOut + uncompressedSize;

    unsigned char block[4096];
    unsigned int iblock = 1;

    unsigned char tagMask = 1;
    unsigned char tag = *pIn++;

    while (pIn < pInEnd) {
        if ((tag & tagMask) == 0) {
            *pOut++       = *pIn;
            block[iblock] = *pIn++;

            iblock = (iblock + 1) & 0xFFF;
        } else {
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

    return TA_TRUE;
}

taBool32 taHPIDecompressZlib(const void* pIn, taUInt32 compressedSize, void* pOut, taUInt32 uncompressedSize)
{
    return mz_uncompress(pOut, &uncompressedSize, pIn, compressedSize) == MZ_OK;
}

void taHPIDecrypt(taUInt8* pData, size_t sizeInBytes, taUInt32 decryptionKey, taUInt32 firstBytePos)
{
    assert(pData != NULL);

    if (decryptionKey != 0) {
        for (size_t i = 0; i < sizeInBytes; ++i) {
            pData[i] = (taUInt8)((firstBytePos + i) ^ decryptionKey) ^ ~pData[i];
        }
    } else {
        for (size_t i = 0; i < sizeInBytes; ++i) {
            pData[i] = pData[i];
        }
    }
}

size_t taHPIReadAndDecrypt(FILE* pFile, void* pBufferOut, size_t bytesToRead, taUInt32 decryptionKey)
{
    if (pFile == NULL) {
        return 0;
    }

    taUInt32 firstBytePos = (taUInt32)taFTell(pFile);

    size_t bytesRead = fread(pBufferOut, 1, bytesToRead, pFile);
    if (decryptionKey != 0) {
        taHPIDecrypt(pBufferOut, bytesRead, decryptionKey, firstBytePos);
    }

    return bytesRead;
}

size_t taHPIReadAndDecryptCompressed(FILE* pFile, void* pBufferOut, size_t uncompressedBytesToRead, taUInt32 decryptionKey)
{
    if (pFile == NULL) {
        return 0;
    }

    size_t chunkCount = uncompressedBytesToRead / 65536;
    if ((uncompressedBytesToRead % 65536) > 0) {
        chunkCount += 1;
    }

    taBool32 result = TA_FALSE;
    size_t* pChunkSizes = malloc(chunkCount * sizeof(*pChunkSizes));

    for (size_t iChunk = 0; iChunk < chunkCount; ++iChunk) {
        if (taHPIReadAndDecrypt(pFile, &pChunkSizes[iChunk], 4, decryptionKey) != 4) {
            result = TA_FALSE;
            goto finished;
        }
    }


    for (size_t iChunk = 0; iChunk < chunkCount; ++iChunk) {
        taUInt32 marker = 0;
        if (taHPIReadAndDecrypt(pFile, &marker, 4, decryptionKey) != 4 || marker != 'HSQS') {
            result = TA_FALSE;
            goto finished;
        }

        taUInt8 unused;
        if (taHPIReadAndDecrypt(pFile, &unused, 1, decryptionKey) != 1) {
            result = TA_FALSE;
            goto finished;
        }

        taUInt8 compressionType;
        if (taHPIReadAndDecrypt(pFile, &compressionType, 1, decryptionKey) != 1) {
            result = TA_FALSE;
            goto finished;
        }

        taUInt8 encrypted;
        if (taHPIReadAndDecrypt(pFile, &encrypted, 1, decryptionKey) != 1) {
            result = TA_FALSE;
            goto finished;
        }

        taUInt32 compressedSize;
        if (taHPIReadAndDecrypt(pFile, &compressedSize, 4, decryptionKey) != 4) {
            result = TA_FALSE;
            goto finished;
        }

        taUInt32 uncompressedSize;
        if (taHPIReadAndDecrypt(pFile, &uncompressedSize, 4, decryptionKey) != 4) {
            result = TA_FALSE;
            goto finished;
        }

        taUInt32 checksum;
        if (taHPIReadAndDecrypt(pFile, &checksum, 4, decryptionKey) != 4) {
            result = TA_FALSE;
            goto finished;
        }


        unsigned char* pCompressedData = malloc(compressedSize);
        if (taHPIReadAndDecrypt(pFile, pCompressedData, compressedSize, decryptionKey) != compressedSize) {
            free(pCompressedData);
            result = TA_FALSE;
            goto finished;
        }

        
        // We do the decryption and checksum in one loop iteration.
        taUInt32 checksum2 = 0;
        if (encrypted) {
            for (taUInt32 i = 0; i < compressedSize; ++i) {
                checksum2 += pCompressedData[i];
                pCompressedData[i] = (pCompressedData[i] - i) ^ i;
            }
        } else {
            for (taUInt32 i = 0; i < compressedSize; ++i) {
                checksum2 += pCompressedData[i];
            }
        }

        if (checksum != checksum2) {
            free(pCompressedData);
            result = TA_FALSE;
            goto finished;
        }


        // The actual decompression.
        switch (compressionType)
        {
            case 1: // LZ77
            {
                if (!taHPIDecompressLZ77(pCompressedData, compressedSize, (char*)pBufferOut + (iChunk * 65536), uncompressedSize)) {
                    free(pCompressedData);
                    result = TA_FALSE;
                    goto finished;
                }
            } break;

            case 2: // Zlib
            {
                if (!taHPIDecompressZlib(pCompressedData, compressedSize, (char*)pBufferOut + (iChunk * 65536), uncompressedSize)) {
                    free(pCompressedData);
                    result = TA_FALSE;
                    goto finished;
                }
            } break;
        }


        free(pCompressedData);
    }

    result = TA_TRUE;


finished:
    free(pChunkSizes);
    return result;
}
