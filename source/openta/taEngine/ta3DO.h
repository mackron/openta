// Copyright (C) 2018 David Reid. See included LICENSE file.

typedef struct ta3DOObject ta3DOObject;

typedef struct
{
    taInt32 x;
    taInt32 y;
    taInt32 z;
} ta3DOVec3;

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
} ta3DOPrimitiveHeader;

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
} ta3DOObjectHeader;

struct ta3DOObject
{
    // The header exactly as specified in 3DO file.
    ta3DOObjectHeader header;

    // The name of the object.
    const char* name;

    // The first child object.
    ta3DOObject* pFirstChild;

    // The next sibling object.
    ta3DOObject* pNextSibling;
};

typedef struct
{
    // A pointer to the file to load from.
    taFile* pFile;

    // The root 3DO object.
    ta3DOObject* pRootObject;
} ta3DO;

// Opens a 3DO object from a file.
ta3DO* taOpen3DO(taFS* pFS, const char* fileName);

// Closes the given 3DO object.
void taClose3DO(ta3DO* p3DO);

// Reads the header of the next object.
//
// This function assumes the file is sitting on the first byte of the object header.
taBool32 ta3DOReadObjectHeader(taFile* pFile, ta3DOObjectHeader* pHeader);

// Loads the primitive header from the given 3DO file.
//
// This function assumes the file is sitting on the first byte of the primitive header.
taBool32 ta3DOReadPrimitiveHeader(taFile* pFile, ta3DOPrimitiveHeader* pHeaderOut);

// Counts the number of objects in the given 3DO file.
//
// This function assumes the file is sitting on the first by the object.
taUInt32 ta3DOCountObjects(taFile* pFile);
