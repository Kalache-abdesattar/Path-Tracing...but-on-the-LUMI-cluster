#ifndef RAY_QUERY_HH
#define RAY_QUERY_HH
#include "bvh.hh"

/* Ray queries are the interface for tracing rays in this program.
 * They are roughly modeled after Vulkan's ray queries.
 *
 * Basic pattern for finding the closest intersection along the ray:
 * ```
 *  ray_query rq = ray_query_initialize(...);
 *  while(ray_query_proceed(&rq))
 *  {
 *      // rq.candidate now contains information on where the ray hit the scene,
 *      // but you still have to confirm it yourself.
 *      if(object_is_not_transparent_or_skipped)
 *          ray_query_confirm(&rq);
 *  }
 *  // rq.closest now contains information on where the ray hit the scene.
 *  // if rq.thit (= hit distance) is negative, the ray missed the scene
 *  // entirely.
 * ```
 */

/* Contains information for a BVH hit. */
typedef struct
{
    /* Barycentric coordinates in the triangle that was hit, used for
     * interpolating triangle vertex parameters.
     */
    float3 barycentrics;
    float thit; /* Hit distance along the ray. */
    uint instance_id; /* Index of the TLAS instance. */
    uint primitive_id; /* Index of the triangle. */
    bool back_face; /* Whether the hit is a back face or not. */
} ray_query_hit_info;

/* Contains information used for traversing a ray in either a BLAS or a TLAS.
 * The traversal for both is done in the exact same function.
 */
typedef struct
{
    /* Handle of the acceleration structure being traversed */
    bvh as;
    /* Ray origin in the acceleration structure's local space (global
     * coordinate system for TLAS, local for BLAS).
     */
    float3 origin;
    /* For a TLAS context, this is just the ray direction in world space.
     * For a BLAS context, it contains a direction-dependent precalculated
     * value which is used in triangle intersection testing.
     */
    float3 dir;
    /* Precalculated 1.0 / dir because division is expensive. */
    float3 inv_dir;
    /* Precalculated offset to the link buffer, saves lots of integer arithmetic
     * in the traversal loop.
     */
    uint link_offset;
    /* Index of the node next in line for traversal. */
    uint node_index;
} ray_query_context;

/* The ray query state object itself. As a user, you should mostly concern
 * yourself with `candidate` and `closest`.
 */
typedef struct
{
    /* Collection of buffers needed during traversal. */
    const bvh_node* node_array;
    const bvh_link* link_array;
    const tlas_instance* instances;
    const uint* mesh_indices;
    const float3* mesh_pos;

    /* Both TLAS and BLAS contexts are active simultaneously: when there's a
     * hit in TLAS, BLAS traversal begins. While that is happening, the TLAS
     * state must be remembered.
     */
    ray_query_context tlas_ctx;
    ray_query_context blas_ctx;
    /* Mesh related to the BLAS being traced currently. Used for fetching actual
     * triangles.
     */
    mesh blas_mesh;


    /* Minimum distance - given by user. Hits closer than this don't count. */
    float tmin;

    /* Maximum distance - initally given by user, but gets limited every time
     * a closer hit is confirmed. Smaller values speed up traversal.
     */
    float tmax;

    /* -1 if not in BLAS, otherwise a precalculated direction-dependent value
     * which is used in triangle intersection testing.
     */
    int blas_axis;

    /* Hit information for the current candidate from ray_query_proceed(). You
     * can use it to check if you want to ray_query_confirm() the hit.
     */
    ray_query_hit_info candidate;

    /* Information for the closest hit from traversal. This is the end result of
     * your ray query.
     */
    ray_query_hit_info closest;
} ray_query;

inline ray_query ray_query_initialize(
    bvh tlas,
    const tlas_instance* instances,
    const bvh_node* node_array,
    const bvh_link* link_array,
    const uint* mesh_indices,
    const float3* mesh_pos,
    float3 origin, float3 direction,
    float tmin, float tmax
){
    ray_query rq;
    rq.node_array = node_array;
    rq.link_array = link_array;
    rq.instances = instances;
    rq.mesh_indices = mesh_indices;
    rq.mesh_pos = mesh_pos;
    rq.tlas_ctx.as = tlas;
    rq.tlas_ctx.origin = origin;
    rq.tlas_ctx.dir = direction;
    rq.tlas_ctx.inv_dir = 1.0f / direction;
    if(direction.x == 0) rq.tlas_ctx.inv_dir.x = 1e40;
    if(direction.y == 0) rq.tlas_ctx.inv_dir.y = 1e40;
    if(direction.z == 0) rq.tlas_ctx.inv_dir.z = 1e40;

    uint directional_link_index = 
        (rq.tlas_ctx.dir.x > 0 ? 1 : 0) |
        (rq.tlas_ctx.dir.y > 0 ? 2 : 0) |
        (rq.tlas_ctx.dir.z > 0 ? 4 : 0);
    rq.tlas_ctx.link_offset =
        tlas.node_offset * 8 + directional_link_index * tlas.node_count;

    rq.tmin = tmin;
    rq.tmax = tmax;
    rq.tlas_ctx.node_index = 0;
    rq.blas_ctx.node_index = 0;
    rq.blas_axis = -1;
    rq.candidate = (ray_query_hit_info){(float3){0,0,0}, -1, 0xFFFFFFFFu, 0, false};
    rq.closest = (ray_query_hit_info){(float3){0,0,0}, -1, 0xFFFFFFFFu, 0, false};

    return rq;
}

inline void ray_query_enter_blas(ray_query* rq, uint index)
{
    tlas_instance instance = rq->instances[index];

    rq->blas_ctx.as = instance.blas;

    float3 o = rq->tlas_ctx.origin;
    float4 origin = (float4){o.x, o.y, o.z, 1};
    origin = mul_m4v4(instance.inv_transform, origin);
    rq->blas_ctx.origin = (float3){origin.x, origin.y, origin.z};

    mat3 vec_transform = extract_m4m3(instance.inv_transform);
    float3 dir = mul_m3v3(vec_transform, rq->tlas_ctx.dir);
    rq->blas_ctx.inv_dir = 1.0f / dir;
    if(dir.x == 0) rq->blas_ctx.inv_dir.x = 1e40;
    if(dir.y == 0) rq->blas_ctx.inv_dir.y = 1e40;
    if(dir.z == 0) rq->blas_ctx.inv_dir.z = 1e40;

    uint directional_link_index = 
        (dir.x > 0 ? 1 : 0) |
        (dir.y > 0 ? 2 : 0) |
        (dir.z > 0 ? 4 : 0);
    rq->blas_ctx.link_offset = instance.blas.node_offset * 8 +
        directional_link_index * instance.blas.node_count;
    rq->blas_ctx.node_index = 0;
    rq->blas_mesh = instance.m;

    ray_triangle_intersection_preprocess(
        dir, &rq->blas_axis, &rq->blas_ctx.dir);
}

inline uint ray_query_traverse(
    ray_query_context* ctx,
    const bvh_node* node_array,
    const bvh_link* link_array,
    float tmin,
    float tmax
){
    const float3 inv_dir = ctx->inv_dir;
    const float3 origin = ctx->origin;
    while(ctx->node_index < ctx->as.node_count)
    {
        bvh_node no = node_array[ctx->as.node_offset + ctx->node_index];
        bvh_link link = link_array[ctx->link_offset + ctx->node_index];
        float3 pmin = (float3){no.min_x, no.min_y, no.min_z};
        float3 pmax = (float3){no.max_x, no.max_y, no.max_z};

        float3 t0 = (pmin - origin) * inv_dir;
        float3 t1 = (pmax - origin) * inv_dir;
        float3 mins = fmin(t0,t1);
        float3 maxs = fmax(t0,t1);
        float near = fmax(mins.x, fmax(mins.y, mins.z));
        float far = fmin(maxs.x, fmin(maxs.y, maxs.z));

        if(near <= far && far > tmin && near < tmax)
        { /* Hit */
            uint accept = link.accept & 0x7FFFFFFFu;
            if(accept != link.accept)
            { /* Leaf node */
                ctx->node_index = link.cancel;
                return accept;
            }
            else ctx->node_index = accept;
        }
        else
        { /* No hit, move to next. */
            ctx->node_index = link.cancel;
        }
    }
    return 0xFFFFFFFFu;
}

inline bool ray_query_test_triangle(ray_query* rq)
{
    mesh m = rq->blas_mesh;
    uint tri_offset = m.index_offset + rq->candidate.primitive_id * 3;
    uint i0 = rq->mesh_indices[tri_offset + 0];
    uint i1 = rq->mesh_indices[tri_offset + 1];
    uint i2 = rq->mesh_indices[tri_offset + 2];
    float3 pos0 = rq->mesh_pos[m.base_vertex_offset + i0];
    float3 pos1 = rq->mesh_pos[m.base_vertex_offset + i1];
    float3 pos2 = rq->mesh_pos[m.base_vertex_offset + i2];

    float3 uvt;
    bool back_face;
    bool hit = ray_triangle_intersection(
        rq->blas_ctx.origin, rq->blas_axis, rq->blas_ctx.dir,
        pos0, pos1, pos2, &uvt, &back_face
    );
    rq->candidate.thit = uvt.z;
    rq->candidate.barycentrics = (float3){uvt.x, uvt.y, 1.0f-uvt.x-uvt.y};
    rq->candidate.back_face = back_face;
    return hit && rq->candidate.thit < rq->tmax && rq->candidate.thit > rq->tmin;
}

inline bool ray_query_proceed(ray_query* rq)
{
    for(;;)
    {
        uint leaf_node = ray_query_traverse(
            rq->blas_axis < 0 ? &rq->tlas_ctx : &rq->blas_ctx,
            rq->node_array, rq->link_array, rq->tmin, rq->tmax
        );

        if(leaf_node != 0xFFFFFFFFu)
        { /* Hit. */
            if(rq->blas_axis < 0)
            { /* Hit a TLAS instance. */
                rq->candidate.instance_id = leaf_node;
                ray_query_enter_blas(rq, leaf_node);
            }
            else
            { /* Hit a BLAS triangle */
                /* Test triangle for hit candidate */
                rq->candidate.primitive_id = leaf_node;
                if(ray_query_test_triangle(rq)) return true;
            }
        }
        else
        { /* No hit. */
            if(rq->blas_axis < 0) return false;
            else rq->blas_axis = -1;
        }
    }
    return false;
}

inline void ray_query_confirm(ray_query* rq)
{
    // Just rq->closest = rq->candidate; would be correct, but that's broken on
    // ROCm for whatever reason that is beyond me.
    rq->closest.barycentrics = rq->candidate.barycentrics;
    rq->closest.thit = rq->candidate.thit;
    rq->closest.instance_id = rq->candidate.instance_id;
    rq->closest.primitive_id = rq->candidate.primitive_id;
    rq->closest.back_face = rq->candidate.back_face;
    rq->tmax = rq->candidate.thit;
}

#endif
