// Copyright (C) 2018 David Reid. See included LICENSE file.

typedef struct ta_3do_object ta_3do_object;

typedef struct
{
    taInt32 x;
    taInt32 y;
    taInt32 z;
} ta_3do_vec3;

typedef struct
{
    taUInt32 colorIndex;
    taUInt32 indexCount;
    taUInt32 unused0;
    taUInt32 indexArrayPtr;
    taUInt32 textureNamePtr;
    taUInt32 unused1;
    taUInt32 unused2;
    taUInt32 isColored;
} ta_3do_primitive_header;

typedef struct
{
    taUInt32 version;
    taUInt32 vertexCount;
    taUInt32 primitiveCount;
    taUInt32 selectionPrimitivePtr; // Only used by the root object.
    taInt32  relativePosX;
    taInt32  relativePosY;
    taInt32  relativePosZ;
    taUInt32 namePtr;
    taUInt32 unused;
    taUInt32 vertexPtr;
    taUInt32 primitivePtr;
    taUInt32 nextSiblingPtr;
    taUInt32 firstChildPtr;
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
taUInt32 ta_3do_count_objects(ta_file* pFile);