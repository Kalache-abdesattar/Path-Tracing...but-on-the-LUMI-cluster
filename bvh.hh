#ifndef BVH_HH
#define BVH_HH
#include "mesh.hh"

/* BVH - Bounding Volume Hierarchy
 *
 * The BVH is an acceleration structure for ray tracing. It is used to quickly
 * find where a ray intersects with the scene, even when the scene has millions
 * of triangles. In short, it is a tree of bounding volumes - shapes that
 * fully contain its child nodes' shapes. `bvh_node` is the node of this tree.
 *
 * All bounding volumes in this implementation are Axis Aligned Bounding Boxes 
 * (AABBs), boxes defined by their extents on the X, Y and Z axes. The leaves
 * of the tree link to the triangles of the 3D meshes.
 *
 * This implementation is a bit abnormal in that it supports stackless tree
 * traversal - `bvh_link`s are used to effectively precalculate where the next
 * node is, depending on whether there was an intersection with the related
 * `bvh_node` or not.
 *
 * BLAS - Bottom Level Acceleration Structure:
 *     Acceleration structure for one mesh on its own. Can contain hundreds of
 *     thousands of triangles and be slow to build. Often precalculated for
 *     static meshes.
 *
 * TLAS - Top Level Acceleration Structure:
 *     Acceleration structure for instances of meshes (= tree built on top of
 *     BLASes). Usually contains (only) thousands of instances, making it fast
 *     to build. It is rebuilt for every animation frame.
 */

/* This structure is a handle to a full BVH. The actual BVH nodes are allocated
 * in `bvh_buffers` instead of in `bvh` directly.
 */
typedef struct
{
    uint node_count; /* Number of nodes in this BVH. */
    uint node_offset; /* Offset of first node of this BVH in `bvh_buffers` */
} bvh;

/* Each node in the BVH is just an AABB around its contained geometry.
 * min_x is the lower bound on the X axis, while max_x is the upper bound, and
 * so on.
 */
typedef struct
{
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;
} bvh_node;

/* One bvh_link corresponds to one bvh_node, they're effectively paired.
 * However, the links are not included in bvh_node because there are 8 different
 * bvh_link sequences, one of which is chosen depending on the ray direction.
 * This allows rays to traverse in roughly near-to-far order, which is faster
 * for finding the closest intersections.
 */
typedef struct
{
    /* Top bit marks if it's a leaf node. In that case, the rest of the bits
     * encode the entry index. Otherwise, this is the index of the node to
     * traverse next if there was an intersection.
     */
    uint accept;

    /* Index of node to traverse next if there was no intersection. */
    uint cancel;
} bvh_link;

/* An instance in the TLAS. Just links a mesh and its related BLAS, with an
 * arbitrary transform matrix. You must fill in the inverse transform matrix
 * as well, it should be exactly `inverse4(transform)` and nothing else, ever!
 */
typedef struct
{
    bvh blas;
    mesh m;
    mat4 transform;
    mat4 inv_transform;
} tlas_instance;

#ifdef __cplusplus
/* All nodes and links for the BVHs are allocated from one continuous vector.
 * This approach is a bit limiting in that you have to load all models up-front.
 *
 * You can still update the TLAS however, as long as it's the last BVH built.
 * Use pop_bvh() to free it, then you can rebuild it without any memory leaks.
 */
struct bvh_buffers
{
    std::vector<bvh_node> nodes;
    std::vector<bvh_link> links;
};

/* Builds a bottom-level acceleration structure for the given mesh, appending
 * the BVH nodes and links in the given bvh_buffers.
 */
bvh build_blas(mesh m, const mesh_buffers& buffers, bvh_buffers& bc);
/* Builds a top-level acceleration structure for the given set of instances and
 * arbitrary indices related to them. Appends the BVH nodes and links in the
 * given bvh_buffers.
 */
bvh build_tlas(
    uint instance_count,
    const std::pair<const tlas_instance*, uint>* ti,
    const bvh_buffers& bc_in,
    bvh_buffers& bc_out
);

/* This function removes the nodes and links of the given BVH, assuming that it
 * is the last entry in bvh_buffers.
 */
void pop_bvh(bvh_buffers& bc, bvh& as);
#endif

#endif
