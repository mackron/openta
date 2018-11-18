// Copyright (C) 2018 David Reid. See included LICENSE file.

#define TA_OPEN_FILE_WITH_NULL_TERMINATOR    0x0001      // Opens a file with a null terminator at the end.

typedef struct taFS taFS;
typedef struct taFile taFile;

enum taSeekOrigin
{
    taSeekOriginStart,
    taSeekOriginCurrent,
    taSeekOriginEnd,
};

typedef struct
{
    // The relative path of the archive on the real file system. This is relative to the executable.
    char relativePath[TA_MAX_PATH];

    // The decryption key.
    taUInt32 decryptionKey;

    // The size in bytes of the central directory.
    taUInt32 centralDirectorySize;

    // The archive's central directory. This is exactly as contained within the archive file, but with the name pointers
    // adjusted such that they can be used as offsets into pCentralDirectory directly. When a file is being loaded, an
    // archive's central directory is used to determine whether or not that file is contained within the archive. By
    // keeping this in memory we can avoid unnecessarilly opening and decrypting the archive for whenever we need to
    // check for a single file.
    char* pCentralDirectory;
} taFSArchive;

struct taFS
{
    // The absolute path of the root directory on the real file system. This is where the executable is stored.
    char rootDir[TA_MAX_PATH];

    // The number of HPI archives in the search list.
    taUInt32 archiveCount;

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
    taFSArchive* pArchives;
};

struct taFile
{
    // The memory stream that's used to from data from the file. Internal use only.
    taMemoryStream _stream;


    // The file system that owns this file.
    taFS* pFS;

    // The size of the file.
    size_t sizeInBytes;

    // The raw uncompressed and unencrypted file data. The rationale for keeping this on the heap is that it greatly
    // simplifies the file system abstraction.
    //
    // The data is intentionally mutable so things can modify the data in-place if necessary.
    char pFileData[1];
};


// Creates a file system instance.
taFS* taCreateFileSystem();

// Deletes the given file system instance.
void taDeleteFileSystem(taFS* pFS);


// Opens the file at the given path from the specified archive file.
taFile* taOpenSpecificFile(taFS* pFS, const char* archiveRelativePath, const char* fileRelativePath, unsigned int options);

// Searches for the given file and opens the first occurance from the highest priority archive.
taFile* taOpenFile(taFS* pFS, const char* relativePath, unsigned int options);

// Closes the given file.
void taCloseFile(taFile* pFile);


// Reads data from the given file.
taBool32 taReadFile(taFile* pFile, void* pBufferOut, size_t bytesToRead, size_t* pBytesReadOut);

// Seeks the given file.
taBool32 taSeekFile(taFile* pFile, taInt64 offset, taSeekOrigin origin);

// Retrieves the current read position of the file.
taUInt64 taTellFile(taFile* pFile);


// High level helper for reading an unsigned 32-bit integer.
taBool32 taReadFileUInt32(taFile* pFile, taUInt32* pBufferOut);

// High level helper for reading a signed 32-bit integer.
taBool32 taReadFileInt32(taFile* pFile, taInt32* pBufferOut);

// High level helper for reading an unsigned 16-bit integer.
taBool32 taReadFileUInt16(taFile* pFile, taUInt16* pBufferOut);

// High level helper for reading an unsigned 8-bit integer.
taBool32 taReadFileUInt8(taFile* pFile, taUInt8* pBufferOut);


// Iteration should work like the following:
//
// taFSIterator* pIter = taFSBegin(pFS, "my/directory");
// while (taFSNext(pIter)) {
//     if (pIter->fileInfo.isDirectory) {
//         taDoSomethingRecursive(pIter->fileInfo.relativePath);
//     } else {
//         taOpenSpecificFile(pIter->pFS, pIter->fileInfo.archiveRelativePath, pIter->fileInfo.relativePath);
//     }
// }
// taFSEnd(pIter);
//
//
// Iteration must be terminated with taFSEnd() at all times, even when the iteration ends naturally. Iteration is not recursive.

typedef struct
{
    // The relative path of the archive that owns this file, if any. This will be empty if the file is sitting on the real file system.
    char archiveRelativePath[TA_MAX_PATH];

    // The relative path of the file.
    char relativePath[TA_MAX_PATH];

    // Whether or not the file is a directory.
    taBool32 isDirectory;
} taFSFileInfo;

typedef struct
{
    // The file system being iterated.
    taFS* pFS;

    // Information about the next file in the iteration. Use this for the path of the file and whatnot.
    taFSFileInfo fileInfo;

    // Variables below are for internal use only.
    size_t _iFile;
    size_t _fileCount;
    taFSFileInfo* _pFiles;
} taFSIterator;

// Begins iterating the contents of the given folder.
taFSIterator* taFSBegin(taFS* pFS, const char* directoryRelativePath, taBool32 recursive);

// Ends the iteration.
void taFSEnd(taFSIterator* pIter);

// Goes to the next file itration.
taBool32 taFSNext(taFSIterator* pIter);



//// HPI Helpers ////

// LZ77 decompression for HPI archives.
taBool32 taHPIDecompressLZ77(const unsigned char* pIn, taUInt32 compressedSize, unsigned char* pOut, taUInt32 uncompressedSize);

// ZLib decompression for HPI archives.
taBool32 taHPIDecompressZlib(const void* pIn, taUInt32 compressedSize, void* pOut, taUInt32 uncompressedSize);

// Decrypts data from a HPI archive.
void taHPIDecrypt(taUInt8* pData, size_t sizeInBytes, taUInt32 decryptionKey, taUInt32 firstBytePos);

// Reads and decrypts data from a HPI archive file.
size_t taHPIReadAndDecrypt(FILE* pFile, void* pBufferOut, size_t bytesToRead, taUInt32 decryptionKey);

// Reads, decrypts and decompresses data from a HPI archive file.
size_t taHPIReadAndDecryptCompressed(FILE* pFile, void* pBufferOut, size_t uncompressedBytesToRead, taUInt32 decryptionKey);
