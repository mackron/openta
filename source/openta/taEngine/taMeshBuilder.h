// Copyright (C) 2018 David Reid. See included LICENSE file.

// The mesh builder provides a simple way to construct mesh geometry.

typedef struct
{
    // The size of an element. This is set when the mesh builder is initialized.
    size_t vertexSize;

    // The size of the vertex buffer, in vertices.
    size_t vertexBufferSize;

    // The number of vertices making up the mesh.
    taUInt32 vertexCount;

    // A pointer to the vertex data.
    void* pVertexData;

    // The size of the index buffer, in taUInt32's.
    size_t indexBufferSize;

    // The number of indices.
    taUInt32 indexCount;

    // A pointer to the index data.
    taUInt32* pIndexData;

    // The texture index for use by loaders. This isn't actually used by the mesh builder, but is included here in order
    // to avoid a wrapper when loading building a mesh from taLoadMap().
    taUInt32 textureIndex;
} taMeshBuilder;

taBool32 taMeshBuilderInit(taMeshBuilder* pBuilder, size_t vertexSize);
void taMeshBuilderUninit(taMeshBuilder* pBuilder);

taBool32 taMeshBuilderWriteVertex(taMeshBuilder* pBuilder, const void* pVertexData);

// Resets the mesh builder.
//
// Do not call taMeshBuilderInit() after calling this, otherwise memory will leak.
void taMeshBuilderReset(taMeshBuilder* pBuilder);
