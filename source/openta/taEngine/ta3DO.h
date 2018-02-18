// Copyright (C) 2018 David Reid. See included LICENSE file.

typedef struct ta_3do_object ta_3do_object;

typedef struct
{
    int32_t x;
    int32_t y;
    int32_t z;
} ta_3do_vec3;

typedef struct
{
    uint32_t colorIndex;
    uint32_t indexCount;
    uint32_t unused0;
    uint32_t indexArrayPtr;
    uint32_t textureNamePtr;
    uint32_t unused1;
    uint32_t unused2;
    uint32_t isColored;
} ta_3do_primitive_header;

typedef struct
{
    uint32_t version;
    uint32_t vertexCount;
    uint32_t primitiveCount;
    uint32_t selectionPrimitivePtr; // Only used by the root object.
    int32_t  relativePosX;
    int32_t  relativePosY;
    int32_t  relativePosZ;
    uint32_t namePtr;
    uint32_t unused;
    uint32_t vertexPtr;
    uint32_t primitivePtr;
    uint32_t nextSiblingPtr;
    uint32_t firstChildPtr;
} ta_3do_object_header;

struct ta_3do_object
{
    // The header exactly as specified in 3DO file.
    ta_3do_object_header header;

    // The name of the object.
    const char* name;

    // The first child object.
    ta_3do_object* pFirstChild;

    // The next sibling object.
    ta_3do_object* pNextSibling;
};

typedef struct
{
    // A pointer to the file to load from.
    ta_file* pFile;

    // The root 3DO object.
    ta_3do_object* pRootObject;

} ta_3do;

// Opens a 3DO object from a file.
ta_3do* ta_open_3do(ta_fs* pFS, const char* fileName);

// Closes the given 3DO object.
void ta_close_3do(ta_3do* p3DO);

// Reads the header of the next object.
//
// This function assumes the file is sitting on the first byte of the object header.
taBool32 ta_3do_read_object_header(ta_file* pFile, ta_3do_object_header* pHeader);

// Loads the primitive header from the given 3DO file.
//
// This function assumes the file is sitting on the first byte of the primitive header.
taBool32 ta_3do_read_primitive_header(ta_file* pFile, ta_3do_primitive_header* pHeaderOut);

// Counts the number of objects in the given 3DO file.
//
// This function assumes the file is sitting on the first by the object.
uint32_t ta_3do_count_objects(ta_file* pFile);