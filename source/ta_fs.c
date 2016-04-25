// Public domain. See "unlicense" statement at the end of this file.

FILE* ta_fopen(const char* filePath, const char* openMode)
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

int ta_fseek(FILE* pFile, int64_t offset, int origin)
{
#ifdef _WIN32
    return _fseeki64(pFile, offset, origin);
#else
    return fseek64(pFile, offset, origin);
#endif
}

int64_t ta_ftell(FILE* pFile)
{
#ifdef _WIN32
    return _ftelli64(pFile);
#else
    return ftell64(pFile);
#endif
}


bool ta_fs__find_file_in_archive(ta_fs* pFS, ta_fs_archive* pArchive, const char* fileRelativePath, uint32_t* pDataPosOut)
{
    assert(pFS != NULL);
    assert(pArchive != NULL);
    assert(fileRelativePath != NULL);
    assert(pDataPosOut != NULL);

    ta_memory_stream stream = ta_create_memory_stream(pArchive->pCentralDirectory, pArchive->centralDirectorySize);

    // Finding the file within the archive is simple - we just search by each path segment in order and keep going until we
    // either find the file or don't find a segment.
    drpath_iterator pathseg;
    if (!drpath_first(fileRelativePath, &pathseg)) {
        return false;
    }

    do
    {
        // At this point we will be sitting on the first byte of the sub-directory.
        uint32_t fileCount;
        if (ta_memory_stream_read(&stream, &fileCount, 4) != 4) {
            return false;
        }

        uint32_t entryOffset;
        if (ta_memory_stream_read(&stream, &entryOffset, 4) != 4) {
            return false;
        }

        bool subdirExists = false;
        for (uint32_t iFile = 0; iFile < fileCount; ++iFile)
        {
            uint32_t namePos;
            if (ta_memory_stream_read(&stream, &namePos, 4) != 4) {
                return false;
            }

            uint32_t dataPos;
            if (ta_memory_stream_read(&stream, &dataPos, 4) != 4) {
                return false;
            }

            uint8_t isDirectory;
            if (ta_memory_stream_read(&stream, &isDirectory, 1) != 1) {
                return false;
            }


            if (_strnicmp(pArchive->pCentralDirectory + namePos, pathseg.path + pathseg.segment.offset, pathseg.segment.length) == 0)
            {
                subdirExists = true;

                // If it's not a directory, but we've still got segments left in the path then there's an error.
                if (pathseg.path[pathseg.segment.offset + pathseg.segment.length] == '\0')
                {
                    // It's the last segment - this is the file we're after.
                    *pDataPosOut = dataPos;
                    return true;
                }
                else
                {
                    if (!isDirectory) {
                        return false;   // We have path segments remaining, but we found a file (not a folder) in the listing. This is erroneous.
                    }

                    // Before continuing we need to move the streamer to the start of the sub-directory.
                    if (!ta_memory_stream_seek(&stream, dataPos, ta_seek_origin_start)) {
                        return false;
                    }
                }

                break;
            }
        }

        if (!subdirExists) {
            return false;
        }

    } while (drpath_next(&pathseg));

    return false;
}


bool ta_fs__file_exists_in_list(const char* relativePath, size_t fileCount, ta_fs_file_info* pFiles)
{
    if (pFiles == NULL) {
        return false;
    }

    for (size_t i = 0; i < fileCount; ++i) {
        if (_stricmp(pFiles[i].relativePath, relativePath) == 0) {    // <-- TA seems to be case insensitive.
            return true;
        }
    }

    return false;
}

void ta_fs__gather_files_in_native_directory(ta_fs* pFS, const char* directoryRelativePath, bool recursive, size_t* pFileCountInOut, ta_fs_file_info** ppFilesInOut)
{
    assert(pFS != NULL);
    assert(directoryRelativePath != NULL);
    assert(pFileCountInOut != NULL);
    assert(ppFilesInOut != NULL);

#ifdef _WIN32
    char searchQuery[DRFS_MAX_PATH];
    if (!drpath_copy_and_append(searchQuery, sizeof(searchQuery), pFS->rootDir, directoryRelativePath)) {
        return;
    }

    unsigned int searchQueryLength = (unsigned int)strlen(searchQuery);
    if (searchQueryLength >= DRFS_MAX_PATH - 3) {
        return;    // Path is too long.
    }

    searchQuery[searchQueryLength + 0] = '\\';
    searchQuery[searchQueryLength + 1] = '*';
    searchQuery[searchQueryLength + 2] = '\0';

    size_t fileCount = 0;

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

        ta_fs_file_info fi;
        fi.archiveRelativePath[0] = '\0';
        drpath_copy_and_append(fi.relativePath, sizeof(fi.relativePath), directoryRelativePath, ffd.cFileName);
        fi.isDirectory = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        // Skip past the file if it's already in the list.
        if (ta_fs__file_exists_in_list(fi.relativePath, *pFileCountInOut, *ppFilesInOut)) {
            continue;
        }



        ta_fs_file_info* pNewFiles = realloc(*ppFilesInOut, ((*pFileCountInOut) + fileCount + 1) * sizeof(**ppFilesInOut));
        if (pNewFiles == NULL) {
            break;
        }

        (*ppFilesInOut) = pNewFiles;
        (*ppFilesInOut)[*pFileCountInOut + fileCount] = fi;
        fileCount += 1;


        if (recursive && ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            ta_fs__gather_files_in_native_directory(pFS, fi.relativePath, recursive, pFileCountInOut, ppFilesInOut);
        }

    } while (FindNextFileA(hFind, &ffd));

    FindClose(hFind);
    *pFileCountInOut += fileCount;
#else
    // TODO: Posix
#endif
}

#if 0
typedef struct
{
    const char* archiveRelativePath;
    size_t prevFileCount;
    size_t* pFileCountInOut;
    ta_fs_file_info** ppFilesInOut;
} ta_fs__gather_files_in_archive_directory_data;

void ta_fs__gather_files_in_archive_directory(ta_fs* pFS, ta_fs_archive* pArchive, const char* directoryRelativePath, bool recursive, size_t* pFileCountInOut, ta_fs_file_info** ppFilesInOut);

bool ta_fs__gather_files_in_archive_directory_traversal_proc(ta_hpi_central_dir_entry* pEntry, const char* filePath, const char* directoryPath, void* pUserData)
{
    ta_fs__gather_files_in_archive_directory_data* pTraversalData = pUserData;
    assert(pTraversalData != NULL);

    size_t prevFileCount = pTraversalData->prevFileCount;
    size_t* pFileCountInOut = pTraversalData->pFileCountInOut;
    ta_fs_file_info** ppFilesInOut = pTraversalData->ppFilesInOut;

    ta_fs_file_info fi;
    strncpy_s(fi.archiveRelativePath, sizeof(fi.archiveRelativePath), pTraversalData->archiveRelativePath, _TRUNCATE);
    drpath_copy_and_append(fi.relativePath, sizeof(fi.relativePath), directoryPath, filePath);
    fi.isDirectory = pEntry->isDirectory;

    // Skip past the file if it's already in the list.
    if (ta_fs__file_exists_in_list(fi.relativePath, prevFileCount, *ppFilesInOut)) {       // <-- use prevFileCount here to make this search more efficient.
        return true;
    }

    ta_fs_file_info* pNewFiles = realloc(*ppFilesInOut, ((*pFileCountInOut) + 1) * sizeof(**ppFilesInOut));
    if (pNewFiles == NULL) {
        return false;
    }

    (*ppFilesInOut) = pNewFiles;
    (*ppFilesInOut)[*pFileCountInOut] = fi;
    *pFileCountInOut += 1;

    return true;
}
#endif

void ta_fs__gather_files_in_archive_directory_at_location(ta_fs* pFS, ta_fs_archive* pArchive, uint32_t directoryDataPos, const char* parentPath, bool recursive, size_t* pFileCountInOut, ta_fs_file_info** ppFilesInOut, uint32_t prevFileCount)
{
    ta_memory_stream stream = ta_create_memory_stream(pArchive->pCentralDirectory, pArchive->centralDirectorySize);
    if (!ta_memory_stream_seek(&stream, directoryDataPos, ta_seek_origin_start)) {
        return;
    }

    uint32_t fileCount;
    if (ta_memory_stream_read(&stream, &fileCount, 4) != 4) {
        return;
    }

    uint32_t entryOffset;
    if (ta_memory_stream_read(&stream, &entryOffset, 4) != 4) {
        return;
    }

    for (uint32_t iFile = 0; iFile < fileCount; ++iFile)
    {
        uint32_t namePos;
        if (ta_memory_stream_read(&stream, &namePos, 4) != 4) {
            return;
        }

        uint32_t dataPos;
        if (ta_memory_stream_read(&stream, &dataPos, 4) != 4) {
            return;
        }

        uint8_t isDirectory;
        if (ta_memory_stream_read(&stream, &isDirectory, 1) != 1) {
            return;
        }


        ta_fs_file_info fi;
        strncpy_s(fi.archiveRelativePath, sizeof(fi.archiveRelativePath), pArchive->relativePath, _TRUNCATE);
        drpath_copy_and_append(fi.relativePath, sizeof(fi.relativePath), parentPath, pArchive->pCentralDirectory + namePos);
        fi.isDirectory = isDirectory;

        // Skip past the file if it's already in the list. Never skip directories.
        if (!fi.isDirectory && ta_fs__file_exists_in_list(fi.relativePath, prevFileCount, *ppFilesInOut)) {       // <-- use prevFileCount here to make this search more efficient.
            continue;
        }


        ta_fs_file_info* pNewFiles = realloc(*ppFilesInOut, ((*pFileCountInOut) + 1) * sizeof(**ppFilesInOut));
        if (pNewFiles == NULL) {
            return;
        }

        (*ppFilesInOut) = pNewFiles;
        (*ppFilesInOut)[*pFileCountInOut] = fi;
        *pFileCountInOut += 1;


        if (recursive && isDirectory) {
            ta_fs__gather_files_in_archive_directory_at_location(pFS, pArchive, dataPos, fi.relativePath, recursive, pFileCountInOut, ppFilesInOut, prevFileCount);
        }
    }
}

void ta_fs__gather_files_in_archive_directory(ta_fs* pFS, ta_fs_archive* pArchive, const char* directoryRelativePath, bool recursive, size_t* pFileCountInOut, ta_fs_file_info** ppFilesInOut)
{
    assert(pFS != NULL);
    assert(directoryRelativePath != NULL);
    assert(pFileCountInOut != NULL);
    assert(ppFilesInOut != NULL);

    // The first thing to do is find the entry within the central directory that represents the directory. 
    uint32_t directoryDataPos;
    if (!ta_fs__find_file_in_archive(pFS, pArchive, directoryRelativePath, &directoryDataPos)) {
        return;
    }

    // We found the directory, so now we need to gather the files within it.
    ta_fs__gather_files_in_archive_directory_at_location(pFS, pArchive, directoryDataPos, directoryRelativePath, recursive, pFileCountInOut, ppFilesInOut, *pFileCountInOut);


#if 0
    char archiveAbsolutePath[DRFS_MAX_PATH];
    if (!drpath_copy_and_append(archiveAbsolutePath, sizeof(archiveAbsolutePath), pFS->rootDir, pArchive->relativePath)) {
        return;
    }

    ta_hpi_archive* pHPI = ta_open_hpi_from_file(archiveAbsolutePath);
    if (pHPI == NULL) {
        return;
    }

    ta_fs__gather_files_in_archive_directory_data traversalData;
    traversalData.archiveRelativePath = pArchive->relativePath;
    traversalData.prevFileCount = *pFileCountInOut;
    traversalData.pFileCountInOut = pFileCountInOut;
    traversalData.ppFilesInOut = ppFilesInOut;
    ta_hpi_traverse_directory(pHPI, directoryRelativePath, recursive, ta_fs__gather_files_in_archive_directory_traversal_proc, &traversalData);

    ta_close_hpi(pHPI);
#endif
}

int ta_fs__file_info_qsort_callback(const void* a, const void* b)
{
    const ta_fs_file_info* pInfoA = a;
    const ta_fs_file_info* pInfoB = b;

    return strcmp(pInfoA->relativePath, pInfoB->relativePath);
}


bool ta_fs__adjust_central_dir_name_pointers_recursive(ta_memory_stream* pStream, uint32_t negativeOffset)
{
    assert(pStream != NULL);

    uint32_t fileCount;
    if (ta_memory_stream_read(pStream, &fileCount, 4) != 4) {
        return false;
    }

    uint32_t entryOffset;
    if (ta_memory_stream_peek(pStream, &entryOffset, 4) != 4) {     // <-- Note that it's a peek and not a read. Reason is because this value is going to be replaced.
        return false;
    }

    // The entry offset is within the central directory. Need to adjust this pointer by simply subtracting negativeOffset.
    assert(entryOffset >= negativeOffset);
    entryOffset -= negativeOffset;
    ta_memory_stream_write_uint32(pStream, entryOffset);

    // Now we need to seek to each file and do the same offsetting for them.
    if (!ta_memory_stream_seek(pStream, entryOffset, ta_seek_origin_start)) {
        return false;
    }

    for (uint32_t i = 0; i < fileCount; ++i)
    {
        uint32_t namePos;
        if (ta_memory_stream_peek(pStream, &namePos, 4) != 4) {     // <-- Note that it's a peek and not a read. Reason is because this value is going to be replaced.
            return false;
        }

        assert(namePos >= negativeOffset);
        namePos -= negativeOffset;
        ta_memory_stream_write_uint32(pStream, namePos);


        uint32_t dataPos;
        if (ta_memory_stream_peek(pStream, &dataPos, 4) != 4) {     // <-- Note that it's a peek and not a read. Reason is because this value is going to be replaced.
            return false;
        }

        assert(dataPos >= negativeOffset);
        dataPos -= negativeOffset;
        ta_memory_stream_write_uint32(pStream, dataPos);


        uint8_t isDirectory;
        if (ta_memory_stream_read(pStream, &isDirectory, 1) != 1) {
            return false;
        }

        // If it's a directory we need to call this recursively. The current read position of the stream needs to be saved and restored in order for
        // the recursion to work correctly.
        if (isDirectory)
        {
            uint32_t currentReadPos = ta_memory_stream_tell(pStream);

            // Before traversing the sub-directory we need to seek to it.
            if (!ta_memory_stream_seek(pStream, dataPos, ta_seek_origin_start)) {
                return false;
            }

            if (!ta_fs__adjust_central_dir_name_pointers_recursive(pStream, negativeOffset)) {
                return false;
            }

            
            // We're done with the sub-directory, but traversing that will have changed the read position of the memory stream, so that will need
            // to be restored before continuing.
            if (!ta_memory_stream_seek(pStream, currentReadPos, ta_seek_origin_start)) {
                return false;
            }
        }
    }

    return true;
}

bool ta_fs__adjust_central_dir_name_pointers(char* pCentralDirectory, uint32_t centralDirectorySize, uint32_t negativeOffset)
{
    assert(pCentralDirectory != NULL);
    assert(negativeOffset > 0);

    ta_memory_stream stream;
    stream.pData = pCentralDirectory;
    stream.dataSize = centralDirectorySize;
    stream.currentReadPos = 0;
    return ta_fs__adjust_central_dir_name_pointers_recursive(&stream, negativeOffset);
}

bool ta_fs__register_archive(ta_fs* pFS, const char* archiveRelativePath)
{
    assert(pFS != NULL);
    assert(archiveRelativePath != NULL);

    // If an archive of the same path already exists, ignore it.
    for (size_t iArchive = 0; iArchive < pFS->archiveCount; ++iArchive) {
        if (_stricmp(pFS->pArchives[iArchive].relativePath, archiveRelativePath) == 0) {    // <-- TA seems to be case insensitive.
            return true;
        }
    }


    // Before registering the archive we want to ensure it's valid. We just read the header for this.
    char archiveAbsolutePath[DRFS_MAX_PATH];
    if (!drpath_copy_and_append(archiveAbsolutePath, sizeof(archiveAbsolutePath), pFS->rootDir, archiveRelativePath)) {
        return false;
    }

    FILE* pFile = ta_fopen(archiveAbsolutePath, "rb");
    if (pFile == NULL) {
        return false;
    }


    // Header
    ta_hpi_header header;
    if (fread(&header, 1, sizeof(header), pFile) != sizeof(header)) {
        fclose(pFile);
        return false;
    }

    if (header.marker != 'IPAH') {
        fclose(pFile);
        return false;    // Not a HPI file.
    }


    // It appears to be a valid archive - add it to the list.
    ta_fs_archive* pNewArchives = realloc(pFS->pArchives, (pFS->archiveCount + 1) * sizeof(*pNewArchives));
    if (pNewArchives == NULL) {
        fclose(pFile);
        return false;
    }

    pFS->pArchives = pNewArchives;


    ta_fs_archive* pArchive = pNewArchives + pFS->archiveCount;
    if (strncpy_s(pArchive->relativePath, sizeof(pArchive->relativePath), archiveRelativePath, _TRUNCATE) != 0) {
        fclose(pFile);
        return false;
    }

    if (header.key == 0) {
        pArchive->decryptionKey = 0;
    } else {
        pArchive->decryptionKey = ~((header.key * 4) | (header.key >> 6));
    }

    // Central directory. A few things here: 1) it starts at header.startPos; 2) it's encrypted; 3) the name pointers need to be adjusted
    // so that they're relative to the start of the central directory data.
    pArchive->centralDirectorySize = header.directorySize - header.startPos;    // <-- Subtract header.startPos because the directory size includes the size of the header.

    if (ta_fseek(pFile, header.startPos, SEEK_SET) != 0) {
        fclose(pFile);
        return false;
    }

    pArchive->pCentralDirectory = malloc(pArchive->centralDirectorySize);
    if (pArchive->pCentralDirectory == NULL) {
        fclose(pFile);
        return false;
    }

    if (fread(pArchive->pCentralDirectory, 1, pArchive->centralDirectorySize, pFile) != pArchive->centralDirectorySize) {
        free(pArchive->pCentralDirectory);
        fclose(pFile);
        return false;
    }
    
    fclose(pFile);


    // The central directory needs to be decrypted.
    ta_hpi_decrypt(pArchive->pCentralDirectory, pArchive->centralDirectorySize, pArchive->decryptionKey, header.startPos);

    // Now we need to recursively traverse the central directory and adjust the pointers to the file names so that they're
    // relative to the central directory. To do this we just offset it by -header.startPos. By doing this now we can simplify
    // future traversals.
    if (!ta_fs__adjust_central_dir_name_pointers(pArchive->pCentralDirectory, pArchive->centralDirectorySize, header.startPos)) {
        free(pArchive->pCentralDirectory);
        return false;
    }

    
    pFS->archiveCount += 1;
    return true;
}


ta_file* ta_fs__open_file_from_archive(ta_fs* pFS, ta_fs_archive* pArchive, const char* fileRelativePath, unsigned int options)
{
    uint32_t dataPos;
    if (!ta_fs__find_file_in_archive(pFS, pArchive, fileRelativePath, &dataPos)) {
        return NULL;
    }

    // It's in this archive.
    char archiveAbsolutePath[DRFS_MAX_PATH];
    if (!drpath_copy_and_append(archiveAbsolutePath, sizeof(archiveAbsolutePath), pFS->rootDir, pArchive->relativePath)) {
        return NULL;
    }

    FILE* pSTDIOFile = ta_fopen(archiveAbsolutePath, "rb");
    if (pSTDIOFile == NULL) {
        return NULL;
    }


    ta_memory_stream stream = ta_create_memory_stream(pArchive->pCentralDirectory, pArchive->centralDirectorySize);
    ta_memory_stream_seek(&stream, dataPos, ta_seek_origin_start);

    uint32_t dataOffset;
    if (ta_memory_stream_read(&stream, &dataOffset, 4) != 4) {
        return NULL;
    }

    uint32_t dataSize;
    if (ta_memory_stream_read(&stream, &dataSize, 4) != 4) {
        return NULL;
    }

    uint8_t compressionType;
    if (ta_memory_stream_read(&stream, &compressionType, 1) != 1) {
        return NULL;
    }


    // Seek to the first byte of the file within the archive.
    if (ta_fseek(pSTDIOFile, dataOffset, ta_seek_origin_start) != 0) {
        return NULL;
    }


    uint32_t extraBytes = 0;
    if (options & TA_OPEN_FILE_WITH_NULL_TERMINATOR) {
        extraBytes = 1;
    }
    

    ta_file* pFile = malloc(sizeof(*pFile) - sizeof(pFile->pFileData) + dataSize + extraBytes);
    if (pFile == NULL) {
        fclose(pSTDIOFile);
        return NULL;
    }

    pFile->_stream = ta_create_memory_stream(pFile->pFileData, dataSize);
    pFile->pFS = pFS;
    pFile->sizeInBytes = dataSize;

    // Here is where we need to read the file data. There is 3 types of compression used.
    //  0 - uncompressed
    //  1 - LZ77
    //  2 - Zlib
    if (compressionType == 0) {
        if (ta_hpi_read_and_decrypt(pSTDIOFile, pFile->pFileData, pFile->sizeInBytes, pArchive->decryptionKey) != pFile->sizeInBytes) {
            fclose(pSTDIOFile);
            free(pFile);
            return NULL;
        }
    } else {
        if (!ta_hpi_read_and_decrypt_compressed(pSTDIOFile, pFile->pFileData, pFile->sizeInBytes, pArchive->decryptionKey)) {
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


#if 0
    char archiveAbsolutePath[DRFS_MAX_PATH];
    if (!drpath_copy_and_append(archiveAbsolutePath, sizeof(archiveAbsolutePath), pFS->rootDir, pArchive->relativePath)) {
        return NULL;
    }

    ta_hpi_archive* pHPI = ta_open_hpi_from_file(archiveAbsolutePath);
    if (pHPI == NULL) {
        return NULL;
    }

    ta_hpi_file* pHPIFile = ta_hpi_open_file(pHPI, fileRelativePath);
    if (pHPIFile == NULL) {
        ta_close_hpi(pHPI);
        return NULL;
    }

    // We have a HPI file.
    ta_file* pFile = malloc(sizeof(*pFile));
    if (pFile == NULL) {
        ta_hpi_close_file(pHPIFile);
        ta_close_hpi(pHPI);
    }

    pFile->pFS = pFS;
    pFile->pHPIFile = pHPIFile;
    pFile->pSTDIOFile = NULL;
        
    return pFile;
#endif
}


ta_fs* ta_create_file_system()
{
    // We need to retrieve the root directory first. We can't continue if this fails.
    char exedir[TA_MAX_PATH];
    if (!dr_get_executable_path(exedir, sizeof(exedir))) {
        return NULL;
    }
    drpath_remove_file_name(exedir);



    ta_fs* pFS = calloc(1, sizeof(*pFS));
    if (pFS == NULL) {
        return NULL;
    }

    if (strcpy_s(pFS->rootDir, sizeof(pFS->rootDir), exedir) != 0) {
        ta_delete_file_system(pFS);
        return NULL;
    }

    pFS->archiveCount = 0;
    pFS->pArchives = NULL;

    // Now we want to try loading all of the known archive files. There are a few mandatory packages which if not present will result
    // in this failing to initialize.
    if (!ta_fs__register_archive(pFS, "rev31.gp3")   ||
        !ta_fs__register_archive(pFS, "totala1.hpi") ||
        !ta_fs__register_archive(pFS, "totala2.hpi"))
    {
        ta_delete_file_system(pFS);
        return false;
    }

    // Optional packages.
    ta_fs__register_archive(pFS, "totala4.hpi");
    ta_fs__register_archive(pFS, "ccdata.ccx");
    ta_fs__register_archive(pFS, "ccmaps.ccx");
    ta_fs__register_archive(pFS, "ccmiss.ccx");
    ta_fs__register_archive(pFS, "btdata.ccx");
    ta_fs__register_archive(pFS, "btmaps.ccx");
    ta_fs__register_archive(pFS, "tactics1.hpi");
    ta_fs__register_archive(pFS, "tactics2.hpi");
    ta_fs__register_archive(pFS, "tactics3.hpi");
    ta_fs__register_archive(pFS, "tactics4.hpi");
    ta_fs__register_archive(pFS, "tactics5.hpi");
    ta_fs__register_archive(pFS, "tactics6.hpi");
    ta_fs__register_archive(pFS, "tactics7.hpi");
    ta_fs__register_archive(pFS, "tactics8.hpi");

    // Now we need to search for .ufo files and register them. We only search the root directory for these.
    size_t fileCount = 0;
    ta_fs_file_info* pFiles = NULL;
    ta_fs__gather_files_in_native_directory(pFS, "", false, &fileCount, &pFiles);
    qsort(pFiles, fileCount, sizeof(*pFiles), ta_fs__file_info_qsort_callback);

    for (size_t iFile = 0; iFile < fileCount; ++iFile) {
        if (!pFiles[iFile].isDirectory && drpath_extension_equal(pFiles[iFile].relativePath, "ufo")) {
            ta_fs__register_archive(pFS, pFiles[iFile].relativePath);
        }
    }
    

    free(pFiles);
    return pFS;
}

void ta_delete_file_system(ta_fs* pFS)
{
    if (pFS == NULL) {
        return;
    }

    if (pFS->pArchives != NULL) {
        for (uint32_t iArchive = 0; iArchive < pFS->archiveCount; ++iArchive) {
            free(pFS->pArchives[iArchive].pCentralDirectory);
        }

        free(pFS->pArchives);
    }

    
    free(pFS);
}


ta_file* ta_open_specific_file(ta_fs* pFS, const char* archiveRelativePath, const char* fileRelativePath, unsigned int options)
{
    if (pFS == NULL) {
        return NULL;
    }


    if (archiveRelativePath == NULL || archiveRelativePath[0] == '\0')
    {
        // No archive file was specified. Try opening from the native file system.
        char fileAbsolutePath[DRFS_MAX_PATH];
        if (!drpath_copy_and_append(fileAbsolutePath, sizeof(fileAbsolutePath), pFS->rootDir, fileRelativePath)) {
            return NULL;
        }

        FILE* pSTDIOFile = ta_fopen(fileAbsolutePath, "rb");
        if (pSTDIOFile == NULL) {
            return NULL;
        }

        uint32_t extraBytes = 0;
        if (options & TA_OPEN_FILE_WITH_NULL_TERMINATOR) {
            extraBytes = 1;
        }


        ta_fseek(pSTDIOFile, 0, SEEK_END);
        uint64_t sizeInBytes = ta_ftell(pSTDIOFile);
        ta_fseek(pSTDIOFile, 0, SEEK_SET);

        if (sizeInBytes > (sizeInBytes - extraBytes)) {
            fclose(pSTDIOFile);
            return NULL;    // File's too big.
        }


        ta_file* pFile = malloc(sizeof(*pFile) - sizeof(pFile->pFileData) + (size_t)sizeInBytes + extraBytes);
        if (pFile == NULL) {
            fclose(pSTDIOFile);
            return NULL;
        }

        pFile->_stream = ta_create_memory_stream(pFile->pFileData, (size_t)sizeInBytes);
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
    }
    else
    {
        // An archive file was specified. Try opening from that archive. To do this we need to search the central directory.
        for (uint32_t iArchive = 0; iArchive < pFS->archiveCount; ++iArchive) {
            if (_stricmp(pFS->pArchives[iArchive].relativePath, archiveRelativePath) == 0) {
                return ta_fs__open_file_from_archive(pFS, &pFS->pArchives[iArchive], fileRelativePath, options);
            }
        }

        return NULL;
    }
}

ta_file* ta_open_file(ta_fs* pFS, const char* relativePath, unsigned int options)
{
    // All we need to do is try opening the file from every archive, in order of priority.

    // The native file system is highest priority.
    ta_file* pFile = ta_open_specific_file(pFS, NULL, relativePath, options);
    if (pFile != NULL) {
        return pFile;   // It's on the native file system.
    }


    // At this point the file is not on the native file system so we just need to search for it.
    for (uint32_t iArchive = 0; iArchive < pFS->archiveCount; ++iArchive) {
        pFile = ta_fs__open_file_from_archive(pFS, &pFS->pArchives[iArchive], relativePath, options);
        if (pFile != NULL) {
            return pFile;
        }
    }


    // Didn't find the file...
    assert(pFile == NULL);
    return NULL;
}

void ta_close_file(ta_file* pFile)
{
    if (pFile == NULL) {
        return;
    }

#if 0
    if (pFile->pHPIFile != NULL) {
        assert(pFile->pSTDIOFile == NULL);
        ta_hpi_close_file(pFile->pHPIFile);
    }

    if (pFile->pSTDIOFile != NULL) {
        assert(pFile->pHPIFile == NULL);
        fclose(pFile->pSTDIOFile);
    }
#endif

    free(pFile);
}


bool ta_read_file(ta_file* pFile, void* pBufferOut, size_t bytesToRead, size_t* pBytesReadOut)
{
    if (pFile == NULL || pBufferOut == NULL) {
        return false;
    }

#if 0
    if (pFile->pHPIFile != NULL)
    {
        assert(pFile->pSTDIOFile == NULL);

        return ta_hpi_read(pFile->pHPIFile, pBufferOut, bytesToRead, pBytesReadOut);
    }
    else
    {
        assert(pFile->pHPIFile == NULL);
        assert(pFile->pSTDIOFile != NULL);

        size_t bytesRead = fread(pBufferOut, 1, bytesToRead, pFile->pSTDIOFile);
        if (pBytesReadOut) {
            *pBytesReadOut = bytesRead;
        }

        return true;
    }
#endif

    size_t bytesRead = ta_memory_stream_read(&pFile->_stream, pBufferOut, bytesToRead);
    if (pBytesReadOut) {
        *pBytesReadOut = bytesRead;
    }

    return true;
}

bool ta_seek_file(ta_file* pFile, int64_t bytesToSeek, ta_seek_origin origin)
{
    if (pFile == NULL) {
        return false;
    }

#if 0
    if (pFile->pHPIFile != NULL)
    {
        assert(pFile->pSTDIOFile == NULL);

        return ta_hpi_seek(pFile->pHPIFile, bytesToSeek, origin);
    }
    else
    {
        assert(pFile->pHPIFile == NULL);
        assert(pFile->pSTDIOFile != NULL);

        int originSTDIO = SEEK_SET;
        switch (origin)
        {
        case ta_seek_origin_current: originSTDIO = SEEK_CUR;
        case ta_seek_origin_end: originSTDIO = SEEK_END;
        default: break;
        }

        return ta_fseek(pFile->pSTDIOFile, bytesToSeek, originSTDIO) == 0;
    }
#endif

    return ta_memory_stream_seek(&pFile->_stream, bytesToSeek, origin);
}

#if 0
void* ta_open_and_read_specific_file__generic_stdio(ta_fs* pFS, const char* archiveRelativePath, const char* fileRelativePath, size_t* pSizeOut, size_t extraBytes)
{
    assert(pFS != NULL);

    // No archive file was specified. Try opening from the native file system.
    char fileAbsolutePath[DRFS_MAX_PATH];
    if (!drpath_copy_and_append(fileAbsolutePath, sizeof(fileAbsolutePath), pFS->rootDir, fileRelativePath)) {
        return NULL;
    }

    FILE* pSTDIOFile = ta_fopen(fileAbsolutePath, "rb");
    if (pSTDIOFile == NULL) {
        return NULL;
    }

    ta_fseek(pSTDIOFile, 0, SEEK_END);
    int64_t sizeInBytes = ta_ftell(pSTDIOFile);
    ta_fseek(pSTDIOFile, 0, SEEK_SET);

    if (sizeInBytes > (SIZE_MAX - extraBytes)) {
        fclose(pSTDIOFile); // File's too big.
        return NULL;
    }

    void* pData = malloc((size_t)sizeInBytes + extraBytes);
    if (pData == NULL) {
        fclose(pSTDIOFile); // Failed to allocate memory. Might have run out.
        return NULL;
    }

    size_t bytesRead = fread(pData, 1, (size_t)sizeInBytes, pSTDIOFile);
    fclose(pSTDIOFile);

    if (pSizeOut) {
        *pSizeOut = bytesRead;
    }

    return pData;
}

void* ta_open_and_read_specific_binary_file(ta_fs* pFS, const char* archiveRelativePath, const char* fileRelativePath, size_t* pSizeOut)
{
    if (pFS == NULL) {
        return NULL;
    }

    if (archiveRelativePath == NULL || archiveRelativePath[0] == '\0')
    {
        // No archive file was specified. Try opening from the native file system.
        return ta_open_and_read_specific_file__generic_stdio(pFS, archiveRelativePath, fileRelativePath, pSizeOut, 0);
    }
    else
    {
        // An archive file was specified. Try opening from that archive.
        char archiveAbsolutePath[DRFS_MAX_PATH];
        if (!drpath_copy_and_append(archiveAbsolutePath, sizeof(archiveAbsolutePath), pFS->rootDir, archiveRelativePath)) {
            return NULL;
        }

        ta_hpi_archive* pHPI = ta_open_hpi_from_file(archiveAbsolutePath);
        if (pHPI == NULL) {
            return NULL;
        }

        void* pFileData = ta_hpi_open_and_read_binary_file(pHPI, fileRelativePath, pSizeOut);
        ta_close_hpi(pHPI);

        return pFileData;
    }
}

char* ta_open_and_read_specific_text_file(ta_fs* pFS, const char* archiveRelativePath, const char* fileRelativePath, size_t* pLengthOut)
{
    if (pFS == NULL) {
        return NULL;
    }

    if (archiveRelativePath == NULL || archiveRelativePath[0] == '\0')
    {
        // No archive file was specified. Try opening from the native file system.
        char* pFileData = ta_open_and_read_specific_file__generic_stdio(pFS, archiveRelativePath, fileRelativePath, pLengthOut, 1);  // <-- '1' means allocate 1 extra byte at the end. Used for the null terminator.
        if (pFileData == NULL) {
            return NULL;
        }

        // Just need to null terminate.
        pFileData[*pLengthOut] = '\0';
        return pFileData;
    }
    else
    {
        // An archive file was specified. Try opening from that archive.
        char archiveAbsolutePath[DRFS_MAX_PATH];
        if (!drpath_copy_and_append(archiveAbsolutePath, sizeof(archiveAbsolutePath), pFS->rootDir, archiveRelativePath)) {
            return NULL;
        }

        ta_hpi_archive* pHPI = ta_open_hpi_from_file(archiveAbsolutePath);
        if (pHPI == NULL) {
            return NULL;
        }

        char* pFileData = ta_hpi_open_and_read_text_file(pHPI, fileRelativePath, pLengthOut);
        ta_close_hpi(pHPI);

        return pFileData;
    }
}


void ta_fs_free(void* pBuffer)
{
    free(pBuffer);
}
#endif


//// Iteration ////

size_t ta_fs__gather_files_in_directory(ta_fs* pFS, const char* directoryRelativePath, ta_fs_file_info** ppFilesInOut, bool recursive)
{
    assert(pFS != NULL);
    assert(directoryRelativePath != NULL);
    assert(ppFilesInOut != NULL);

    size_t fileCount = 0;

    // Native directory has the highest priority.
    ta_fs__gather_files_in_native_directory(pFS, directoryRelativePath, recursive, &fileCount, ppFilesInOut);
    
    // Archives after the native directory.
    for (uint32_t iArchive = 0; iArchive < pFS->archiveCount; ++iArchive) {
        ta_fs__gather_files_in_archive_directory(pFS, &pFS->pArchives[iArchive], directoryRelativePath, recursive, &fileCount, ppFilesInOut);
    }

    return fileCount;
}

ta_fs_iterator* ta_fs_begin(ta_fs* pFS, const char* directoryRelativePath, bool recursive)
{
    if (pFS == NULL) {
        return NULL;
    }

    // If the directory path is null, just treat it as though we are iteration over the root directory.
    if (directoryRelativePath == NULL) {
        directoryRelativePath = "";
    }

    ta_fs_iterator* pIter = calloc(1, sizeof(*pIter));
    if (pIter == NULL) {
        return NULL;
    }

    pIter->pFS = pFS;
    pIter->_fileCount = ta_fs__gather_files_in_directory(pFS, directoryRelativePath, &pIter->_pFiles, recursive);

    return pIter;
}

void ta_fs_end(ta_fs_iterator* pIter)
{
    if (pIter == NULL) {
        return;
    }

    free(pIter->_pFiles);
    free(pIter);
}

bool ta_fs_next(ta_fs_iterator* pIter)
{
    if (pIter == NULL || pIter->_iFile >= pIter->_fileCount) {
        return false;
    }

    assert(pIter->_pFiles != NULL);

    pIter->fileInfo = pIter->_pFiles[pIter->_iFile];
    pIter->_iFile += 1;

    return true;
}



//// HPI Helpers ////

bool ta_hpi_decompress_lz77(const unsigned char* pIn, uint32_t compressedSize, unsigned char* pOut, uint32_t uncompressedSize)
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

bool ta_hpi_decompress_zlib(const void* pIn, uint32_t compressedSize, void* pOut, uint32_t uncompressedSize)
{
    return mz_uncompress(pOut, &uncompressedSize, pIn, compressedSize) == MZ_OK;
}

void ta_hpi_decrypt(uint8_t* pData, size_t sizeInBytes, uint32_t decryptionKey, uint32_t firstBytePos)
{
    assert(pData != NULL);

    if (decryptionKey != 0) {
        for (size_t i = 0; i < sizeInBytes; ++i) {
            pData[i] = (uint8_t)((firstBytePos + i) ^ decryptionKey) ^ ~pData[i];
        }
    } else {
        for (size_t i = 0; i < sizeInBytes; ++i) {
            pData[i] = pData[i];
        }
    }
}

size_t ta_hpi_read_and_decrypt(FILE* pFile, void* pBufferOut, size_t bytesToRead, uint32_t decryptionKey)
{
    if (pFile == NULL) {
        return 0;
    }

    uint32_t firstBytePos = (uint32_t)ta_ftell(pFile);

    size_t bytesRead = fread(pBufferOut, 1, bytesToRead, pFile);
    if (decryptionKey != 0) {
        ta_hpi_decrypt(pBufferOut, bytesRead, decryptionKey, firstBytePos);
    }

    return bytesRead;
}

size_t ta_hpi_read_and_decrypt_compressed(FILE* pFile, void* pBufferOut, size_t uncompressedBytesToRead, uint32_t decryptionKey)
{
    if (pFile == NULL) {
        return 0;
    }

    size_t chunkCount = uncompressedBytesToRead / 65536;
    if ((uncompressedBytesToRead % 65536) > 0) {
        chunkCount += 1;
    }

    bool result = false;
    size_t* pChunkSizes = malloc(chunkCount * sizeof(*pChunkSizes));

    for (size_t iChunk = 0; iChunk < chunkCount; ++iChunk)
    {
        if (ta_hpi_read_and_decrypt(pFile, &pChunkSizes[iChunk], 4, decryptionKey) != 4) {
            result = false;
            goto finished;
        }
    }


    for (size_t iChunk = 0; iChunk < chunkCount; ++iChunk)
    {
        uint32_t marker = 0;
        if (ta_hpi_read_and_decrypt(pFile, &marker, 4, decryptionKey) != 4 || marker != 'HSQS') {
            result = false;
            goto finished;
        }

        uint8_t unused;
        if (ta_hpi_read_and_decrypt(pFile, &unused, 1, decryptionKey) != 1) {
            result = false;
            goto finished;
        }

        uint8_t compressionType;
        if (ta_hpi_read_and_decrypt(pFile, &compressionType, 1, decryptionKey) != 1) {
            result = false;
            goto finished;
        }

        uint8_t encrypted;
        if (ta_hpi_read_and_decrypt(pFile, &encrypted, 1, decryptionKey) != 1) {
            result = false;
            goto finished;
        }

        uint32_t compressedSize;
        if (ta_hpi_read_and_decrypt(pFile, &compressedSize, 4, decryptionKey) != 4) {
            result = false;
            goto finished;
        }

        uint32_t uncompressedSize;
        if (ta_hpi_read_and_decrypt(pFile, &uncompressedSize, 4, decryptionKey) != 4) {
            result = false;
            goto finished;
        }

        uint32_t checksum;
        if (ta_hpi_read_and_decrypt(pFile, &checksum, 4, decryptionKey) != 4) {
            result = false;
            goto finished;
        }


        unsigned char* pCompressedData = malloc(compressedSize);
        if (ta_hpi_read_and_decrypt(pFile, pCompressedData, compressedSize, decryptionKey) != compressedSize) {
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
                if (!ta_hpi_decompress_lz77(pCompressedData, compressedSize, (char*)pBufferOut + (iChunk * 65536), uncompressedSize)) {
                    free(pCompressedData);
                    result = false;
                    goto finished;
                }
            } break;

            case 2: // Zlib
            {
                if (!ta_hpi_decompress_zlib(pCompressedData, compressedSize, (char*)pBufferOut + (iChunk * 65536), uncompressedSize)) {
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
