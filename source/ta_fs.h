// Public domain. See "unlicense" statement at the end of this file.

typedef enum
{
    ta_seek_origin_start,
    ta_seek_origin_current,
    ta_seek_origin_end,
} ta_seek_origin;

typedef struct
{
    // The relative path of the archive on the real file system. This is relative to the executable.
    char relativePath[TA_MAX_PATH];

} ta_fs_archive;

struct ta_fs
{
    // The absolute path of the root directory on the real file system. This is where the executable is stored.
    char rootDir[TA_MAX_PATH];

    // The number of HPI archives in the search list.
    uint32_t archiveCount;

    // The list of HPI archives in the search list. These are listed in order of priority, and must include at least
    // the following, in order of priority:
    //   rev31.gp3
    //   totala1.hpi
    //   totala2.hpi
    //
    // The list will also usually include the following optional packages, but are not required.
    //   totala4.hpi (single player content)
    //   ccdata.hpi, ccmaps.hpi, ccmiss.hpi (Core Contingency content)
    //   btdata.hpi, btmaps.hpi, tactics*.hpi (Battle Tactics content)
    //   *.ufo
    //
    // Note that totala3.hpi and worlds.hpi are completely ignored.
    ta_fs_archive* pArchives;
};

struct ta_file
{
    // The file system that owns this file.
    ta_fs* pFS;

    // A handle to the file if it is contained within a HPI archive. If it's a file on the real file system this will
    // be set to NULL and pSTDIOFile will be non-null.
    ta_hpi_file* pHPIFile;

    // A handle to the file if it is contained within the real file system.
    FILE* pSTDIOFile;
};


// Creates a file system instance.
ta_fs* ta_create_file_system();

// Deletes the given file system instance.
void ta_delete_file_system(ta_fs* pFS);


// Opens the file at the given path from the specified archive file.
ta_file* ta_open_specific_file(ta_fs* pFS, const char* archiveRelativePath, const char* fileRelativePath);

// Searches for the given file and opens the first occurance from the highest priority archive.
ta_file* ta_open_file(ta_fs* pFS, const char* relativePath);

// Closes the given file.
void ta_close_file(ta_file* pFile);


// Reads data from the given file.
bool ta_read_file(ta_file* pFile, void* pBufferOut, size_t bytesToRead, size_t* pBytesReadOut);

// Seeks the given file.
bool ta_seek_file(ta_file* pFile, int64_t offset, ta_seek_origin origin);


// Iteration should work like the following:
//
// ta_fs_iterator* pIter = ta_fs_begin(pFS, "my/directory");
// while (ta_fs_next(pIter)) {
//     if (pIter->fileInfo.isDirectory) {
//         ta_do_something_recursive(pIter->fileInfo.relativePath);
//     } else {
//         ta_open_specific_file(pIter->pFS, pIter->fileInfo.archiveRelativePath, pIter->fileInfo.relativePath);
//     }
// }
// ta_fs_end(pIter);
//
//
// Iteration must be terminated with ta_fs_end() at all times, even when the iteration ends naturally. Iteration is not recursive.

typedef struct
{
    // The relative path of the archive that owns this file, if any. This will be empty if the file is sitting on the real file system.
    char archiveRelativePath[TA_MAX_PATH];

    // The relative path of the file.
    char relativePath[TA_MAX_PATH];

    // Whether or not the file is a directory.
    bool isDirectory;

} ta_fs_file_info;

typedef struct
{
    // The file system being iterated.
    ta_fs* pFS;

    // Information about the next file in the iteration. Use this for the path of the file and whatnot.
    ta_fs_file_info fileInfo;


    // Variables below are for internal use only.
    size_t _iFile;
    size_t _fileCount;
    ta_fs_file_info* _pFiles;

} ta_fs_iterator;

// Begins iterating the contents of the given folder.
ta_fs_iterator* ta_fs_begin(ta_fs* pFS, const char* directoryRelativePath);

// Ends the iteration.
void ta_fs_end(ta_fs_iterator* pIter);

// Goes to the next file itration.
bool ta_fs_next(ta_fs_iterator* pIter);