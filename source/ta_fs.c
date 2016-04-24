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

bool ta_fs__file_exists_in_list(const char* relativePath, size_t fileCount, ta_fs_file_info* pFiles)
{
    assert(pFiles != NULL);

    for (size_t i = 0; i < fileCount; ++i) {
        if (_stricmp(pFiles[i].relativePath, relativePath) == 0) {    // <-- TA seems to be case insensitive.
            return true;
        }
    }

    return false;
}

void ta_fs__gather_files_in_native_directory(ta_fs* pFS, const char* directoryRelativePath, size_t* pFileCountInOut, ta_fs_file_info** ppFilesInOut)
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

        // Skip past the file if it's already in the list.
        if (ta_fs__file_exists_in_list(ffd.cFileName, *pFileCountInOut, *ppFilesInOut)) {
            continue;
        }


        ta_fs_file_info fi;

        fi.archiveRelativePath[0] = '\0';
        strncpy_s(fi.relativePath, sizeof(fi.relativePath), ffd.cFileName, _TRUNCATE);
            
        fi.isDirectory = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        ULARGE_INTEGER liSize;
        liSize.LowPart  = ffd.nFileSizeLow;
        liSize.HighPart = ffd.nFileSizeHigh;
        fi.sizeInBytes = liSize.QuadPart;


        ta_fs_file_info* pNewFiles = realloc(*ppFilesInOut, ((*pFileCountInOut) + fileCount + 1) * sizeof(**ppFilesInOut));
        if (pNewFiles == NULL) {
            break;
        }

        (*ppFilesInOut) = pNewFiles;
        (*ppFilesInOut)[*pFileCountInOut + fileCount] = fi;
        fileCount += 1;

    } while (FindNextFileA(hFind, &ffd));

    FindClose(hFind);
    *pFileCountInOut += fileCount;
#else
    // TODO: Posix
#endif
}

void ta_fs__gather_files_in_archive_directory(ta_fs* pFS, const char* archiveRelativePath, const char* directoryRelativePath, size_t* pFileCountInOut, ta_fs_file_info** ppFilesInOut)
{
    assert(pFS != NULL);
    assert(directoryRelativePath != NULL);
    assert(pFileCountInOut != NULL);
    assert(ppFilesInOut != NULL);


}

int ta_fs__file_info_qsort_callback(const void* a, const void* b)
{
    const ta_fs_file_info* pInfoA = a;
    const ta_fs_file_info* pInfoB = b;

    return strcmp(pInfoA->relativePath, pInfoB->relativePath);
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

    // Just a quick validation based on the header.
    uint32_t header;
    if (fread(&header, 1, sizeof(header), pFile) != sizeof(header)) {
        fclose(pFile);
        return false;
    }

    fclose(pFile);


    // It appears to be a valid archive - add it to the list.
    ta_fs_archive* pNewArchives = realloc(pFS->pArchives, (pFS->archiveCount + 1) * sizeof(*pNewArchives));
    if (pNewArchives == NULL) {
        return false;
    }

    pFS->pArchives = pNewArchives;
    if (strncpy_s(pFS->pArchives[pFS->archiveCount].relativePath, sizeof(pFS->pArchives[pFS->archiveCount].relativePath), archiveRelativePath, _TRUNCATE) != 0) {
        return false;
    }

    pFS->archiveCount += 1;
    return true;
}


ta_fs* ta_create_file_system()
{
    // We need to retrieve the root directory first. We can't continue if this fails.
    char exedir[TA_MAX_PATH];
    if (!dr_get_executable_path(exedir, sizeof(exedir))) {
        return NULL;
    }
    drpath_remove_file_name(exedir);

#if 0
#ifdef _WIN32
    _chdir(exedir);
#else
    chdir(exedir);
#endif
#endif


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
    ta_fs__gather_files_in_native_directory(pFS, "", &fileCount, &pFiles);
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

    free(pFS->pArchives);
    free(pFS);
}


ta_file* ta_open_specific_file(ta_fs* pFS, const char* archiveRelativePath, const char* fileRelativePath)
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

        // We have a native file.
        ta_file* pFile = malloc(sizeof(*pFile));
        if (pFile == NULL) {
            fclose(pSTDIOFile);
            return NULL;
        }

        pFile->pFS = pFS;
        pFile->pHPIFile = NULL;
        pFile->pSTDIOFile = pSTDIOFile;

        return pFile;
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
    }
}

ta_file* ta_open_file(ta_fs* pFS, const char* relativePath)
{
    // All we need to do is try opening the file from every archive, in order of priority.

    // The native file system is highest priority.
    ta_file* pFile = ta_open_specific_file(pFS, NULL, relativePath);
    if (pFile != NULL) {
        return pFile;   // It's on the native file system.
    }


    // At this point the file is not on the native file system so we just need to search for it.
    for (uint32_t iArchive = 0; iArchive < pFS->archiveCount; ++iArchive) {
        pFile = ta_open_specific_file(pFS, pFS->pArchives[iArchive].relativePath, relativePath);
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

    if (pFile->pHPIFile != NULL) {
        assert(pFile->pSTDIOFile == NULL);
        ta_hpi_close_file(pFile->pHPIFile);
    }

    if (pFile->pSTDIOFile != NULL) {
        assert(pFile->pHPIFile == NULL);
        fclose(pFile->pSTDIOFile);
    }

    free(pFile);
}


bool ta_read_file(ta_file* pFile, void* pBufferOut, size_t bytesToRead, size_t* pBytesReadOut)
{
    if (pFile == NULL || pBufferOut == NULL) {
        return false;
    }

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
}

bool ta_seek_file(ta_file* pFile, int64_t bytesToSeek, ta_seek_origin origin)
{
    if (pFile == NULL) {
        return false;
    }

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

#ifdef _WIN32
        return _fseeki64(pFile->pSTDIOFile, bytesToSeek, originSTDIO) == 0;
#else
        return fseek64(pFile->pSTDIOFile, bytesToSeek, originSTDIO) == 0;
#endif
    }
}


//// Iteration ////

size_t ta_fs__gather_files_in_directory(ta_fs* pFS, const char* directoryRelativePath, ta_fs_file_info** ppFilesInOut)
{
    assert(pFS != NULL);
    assert(directoryRelativePath != NULL);
    assert(ppFilesInOut != NULL);

    size_t fileCount = 0;

    // Native directory has the highest priority.
    ta_fs__gather_files_in_native_directory(pFS, directoryRelativePath, &fileCount, ppFilesInOut);
    
    // Archives after the native directory.
    for (uint32_t iArchive = 0; iArchive < pFS->archiveCount; ++iArchive) {
        ta_fs__gather_files_in_archive_directory(pFS, pFS->pArchives[iArchive].relativePath, directoryRelativePath, &fileCount, ppFilesInOut);
    }

    return fileCount;
}

ta_fs_iterator* ta_fs_begin(ta_fs* pFS, const char* directoryRelativePath)
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

    pIter->_fileCount = ta_fs__gather_files_in_directory(pFS, directoryRelativePath, &pIter->_pFiles);

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