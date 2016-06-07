
// The mesh builder provides a simple way to construct mesh geometry.

typedef struct
{
    // The size of an element. This is set when the mesh builder is initialized.
    size_t vertexSize;

    // The size of the vertex buffer, in vertices.
    size_t vertexBufferSize;

    // The number of vertices making up the mesh.
    uint32_t vertexCount;

    // A pointer to the vertex data.
    void* pVertexData;

    // The texture index for use by loaders. This isn't actually used by the mesh builder, but is included here in order
    // to avoid a wrapper when loading building a mesh from ta_load_map().
    uint32_t textureIndex;

} ta_mesh_builder;

bool ta_mesh_builder_init(ta_mesh_builder* pBuilder, size_t vertexSize);
void ta_mesh_builder_uninit(ta_mesh_builder* pBuilder);

bool ta_mesh_builder_write_vertex(ta_mesh_builder* pBuilder, const void* pVertexData);

// Resets the mesh builder.
//
// Do not call ta_mesh_builder_init() after calling this, otherwise memory will leak.
void ta_mesh_builder_reset(ta_mesh_builder* pBuilder);