// Public domain. See "unlicense" statement at the end of this file.

// Callback for when data is read. Return value is the number of bytes actually read.
typedef size_t (* ta_read_proc)(void* userData, void* bufferOut, size_t bytesToRead);

// Callback for when data needs to be seeked. Return value is whether or not seeking was successful. The offset is relative to the start.
typedef bool (* ta_seek_proc)(void* userData, uint32_t offsetFromStart);


typedef struct
{
    uint32_t marker;
    uint32_t saveMarker;
    uint32_t directorySize;
    uint32_t key;
    uint32_t startPos;
} ta_hpi_header;

typedef struct
{
    // The name of the file. This is an offset of ta_hpi_archive::pExtraData.
    const char* pName;

    // The position of the first byte of the file data in the archive.
    uint32_t dataPos;

    // The size of the file.
    uint32_t sizeInBytes;

    // Whether or not the file is a directory.
    bool isDirectory;

} ta_hpi_file_info;

// Structure representing a HPI archive.
typedef struct
{
    // onRead callback.
    ta_read_proc onRead;

    // onSeek callback.
    ta_seek_proc onSeek;

    // The user data that's passed to onRead and onSeek.
    void* pUserData;


    // The header.
    ta_hpi_header header;

    // The current read position. We need this to decrypt.
    uint32_t currentPos;

    // The main decryption key.
    uint32_t decryptionKey;


    // Variable length data containing information about each file in the archive.
    char pExtraData[1];

} ta_hpi_archive;

// Structure representing a file within a HPI archive.
typedef struct ta_hpi_file ta_hpi_file;
struct ta_hpi_file
{
    // A pointer the archive that owns this file.
    ta_hpi_archive* pHPI;

    // The size of the file.
    uint32_t sizeInBytes;

    // The current position of the file's read pointer.
    uint32_t readPointer;

    // The raw, uncompressed file data. For simplicity we just load the entire file at once onto the heap. This should not
    // be an issue on modern hardware.
    unsigned char pFileData[1];
};


// Opens the HPI file from the given callbacks.
//
// This function assumes the first bytes read from these callbacks are of the first bytes of the HPI file.
ta_hpi_archive* ta_open_hpi(ta_read_proc onRead, ta_seek_proc onSeek, void* pUserData);

// A helper for opening a HPI archive from a file.
ta_hpi_archive* ta_open_hpi_from_file(const char* filename);

// Closes the given HPI archive.
void ta_close_hpi(ta_hpi_archive* pHPI);


// Opens a file.
ta_hpi_file* ta_hpi_open_file(ta_hpi_archive* pHPI, const char* fileName);

// Closes the given HPI file.
void ta_hpi_close_file(ta_hpi_file* pFile);


// Reads data from the given file.
bool ta_hpi_read(ta_hpi_file* pFile, void* pBufferOut, size_t bytesToRead, size_t* pBytesReadOut);

// Seeks the given file.
bool ta_hpi_seek(ta_hpi_file* pFile, int64_t bytesToSeek, ta_seek_origin origin);

// Retrieves the current read pointer.
uint64_t ta_hpi_tell(ta_hpi_file* pFile);

// Retrieves the size of the given file, in bytes.
uint64_t ta_hpi_size(ta_hpi_file* pFile);
