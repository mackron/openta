// Copyright (C) 2018 David Reid. See included LICENSE file.

#define TA_OPEN_FILE_WITH_NULL_TERMINATOR    0x0001      // Opens a file with a null terminator at the end.

enum ta_seek_origin
{
    ta_seek_origin_start,
    ta_seek_origin_current,
    ta_seek_origin_end,
};

typedef struct
{
    // The relative path of the archive on the real file system. This is relative to the executable.
    char relativePath[TA_MAX_PATH];

    // The decryption key.
    uint32_t decryptionKey;

    // The size in bytes of the central directory.
    uint32_t centralDirectorySize;

    // The archive's central directory. This is exactly as contained within the archive file, but with the name pointers
    // adjusted such that they can be used as offsets into pCentralDirectory directly. When a file is being loaded, an
    // archive's central directory is used to determine whether or not that file is contained within the archive. By
    // keeping this in memory we can avoid unnecessarilly opening and decrypting the archive for whenever we need to
    // check for a single file.
    char* pCentralDirectory;

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
    // The memory stream that's used to from data from the file. Internal use only.
    ta_memory_stream _stream;


    // The file system that owns this file.
    ta_fs* pFS;

    // The size of the file.
    size_t sizeInBytes;

    // The raw uncompressed and unencrypted file data. The rationale for keeping this on the heap is that it greatly
    // simplifies the file system abstraction.
    //
    // The data is intentionally mutable so things can modify the data in-place if necessary.
    char pFileData[1];
};


// Creates a file system instance.
ta_fs* ta_create_file_system();

// Deletes the given file system instance.
void ta_delete_file_system(ta_fs* pFS);


// Opens the file at the given path from the specified archive file.
ta_file* ta_open_specific_file(ta_fs* pFS, const char* archiveRelativePath, const char* fileRelativePath, unsigned int options);

// Searches for the given file and opens the first occurance from the highest priority archive.
ta_file* ta_open_file(ta_fs* pFS, const char* relativePath, unsigned int options);

// Closes the given file.
void ta_close_file(ta_file* pFile);


// Reads data from the given file.
ta_bool32 ta_read_file(ta_file* pFile, void* pBufferOut, size_t bytesToRead, size_t* pBytesReadOut);

// Seeks the given file.
ta_bool32 ta_seek_file(ta_file* pFile, int64_t offset, ta_seek_origin origin);

// Retrieves the current read position of the file.
uint64_t ta_tell_file(ta_file* pFile);


// High level helper for reading an unsigned 32-bit integer.
ta_bool32 ta_read_file_uint32(ta_file* pFile, uint32_t* pBufferOut);

// High level helper for reading a signed 32-bit integer.
ta_bool32 ta_read_file_int32(ta_file* pFile, int32_t* pBufferOut);

// High level helper for reading an unsigned 16-bit integer.
ta_bool32 ta_read_file_uint16(ta_file* pFile, uint16_t* pBufferOut);

// High level helper for reading an unsigned 8-bit integer.
ta_bool32 ta_read_file_uint8(ta_file* pFile, uint8_t* pBufferOut);


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
    ta_bool32 isDirectory;

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
ta_fs_iterator* ta_fs_begin(ta_fs* pFS, const char* directoryRelativePath, ta_bool32 recursive);

// Ends the iteration.
void ta_fs_end(ta_fs_iterator* pIter);

// Goes to the next file itration.
ta_bool32 ta_fs_next(ta_fs_iterator* pIter);



//// HPI Helpers ////

// LZ77 decompression for HPI archives.
ta_bool32 ta_hpi_decompress_lz77(const unsigned char* pIn, uint32_t compressedSize, unsigned char* pOut, uint32_t uncompressedSize);

// ZLib decompression for HPI archives.
ta_bool32 ta_hpi_decompress_zlib(const void* pIn, uint32_t compressedSize, void* pOut, uint32_t uncompressedSize);

// Decrypts data from a HPI archive.
void ta_hpi_decrypt(uint8_t* pData, size_t sizeInBytes, uint32_t decryptionKey, uint32_t firstBytePos);

// Reads and decrypts data from a HPI archive file.
size_t ta_hpi_read_and_decrypt(FILE* pFile, void* pBufferOut, size_t bytesToRead, uint32_t decryptionKey);

// Reads, decrypts and decompresses data from a HPI archive file.
size_t ta_hpi_read_and_decrypt_compressed(FILE* pFile, void* pBufferOut, size_t uncompressedBytesToRead, uint32_t decryptionKey);
