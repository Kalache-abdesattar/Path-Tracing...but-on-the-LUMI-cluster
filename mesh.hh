#ifndef MESH_HH
#define MESH_HH
#include "math.hh"

/* Meshes define the shape of a 3D model. They are a collection of vertices,
 * organized in triangles. Since this program does not support textures, they
 * also contain all material information as well, so they're equivalent to a
 * full 3D model here.
 *
 * This program also uses indexed meshes; this means that triangles are defined
 * as a set of 3 indices to a list of vertices. This allows sharing vertices in
 * multiple triangles without duplicating them in memory.
 */

/* A handle to a mesh. The actual vertex data and triangle indices are stored
 * in `mesh_buffers`.
 */
typedef struct
{
    uint vertex_count;
    uint triangle_count;
    /* Offset of first triangle index in mesh_buffer. */
    uint index_offset;
    /* Offset of first vertex in mesh_buffers. Triangle indices are relative to
     * this.
     */
    uint base_vertex_offset;
} mesh;

#ifdef __cplusplus
#include <vector>
struct mesh_buffers
{
    /* Index buffer, contains 3 indices per triangle. */
    std::vector<uint> indices;

    /* Vertex buffers. */
    std::vector<float3> pos; /* 3D coordinate */
    std::vector<float3> normal; /* Normal vector */
    /* XYZ = RGB base color, W = alpha. */
    std::vector<float4> albedo;
    /* X = roughness, Y = metallicness, Z = transmission, W = emission */
    std::vector<float4> material;
};

/* Loads a mesh from a .obj file. Does not support non-triangular faces, so make
 * sure you export your mesh triangulated! The indices and vertices from the
 * mesh are allocated in mesh_buffers.
 */
mesh load_mesh(mesh_buffers& mb, const char* obj_file);
#endif

#endif
