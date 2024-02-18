#include "path_tracer.hh"
#include "scene.hh"



__kernel void baseline_render(
    __global subframe *subframes,
    __global tlas_instance *instances,
    __global bvh_node *bvh_nodes,
    __global bvh_link *bvh_links,
    __global uint *mesh_indices,
    __global float3 *mesh_pos,
    __global float3 *mesh_normal,
    __global float4 *mesh_albedo,
    __global float4 *mesh_material,

    //const uint IMAGE_WIDTH,
    //const uint IMAGE_HEIGHT,

    __global uchar4 *image
){        
    // Get the work-item’s unique ID
    const uint x = get_global_id(0);
    const uint y = get_global_id(1);
    
    float3 color = {0, 0, 0};

    for(uint j = 0; j < SAMPLES_PER_PIXEL; ++j)
    {
        color += path_trace_pixel(
            (uint2){x, y},
            j,
            subframes,
            instances,
            bvh_nodes,
            bvh_links,
            mesh_indices,
            mesh_pos,
            mesh_normal,
            mesh_albedo,
            mesh_material
        );
    
    }
        
    color /= SAMPLES_PER_PIXEL;
    //image[IMAGE_WIDTH * y + x] = color; //tonemap_pixel(color);
}