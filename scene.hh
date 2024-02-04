#ifndef SCENE_HH
#define SCENE_HH
#include "mesh.hh"
#include "bvh.hh"
#include "config.hh"

typedef struct
{
    mat3 orientation;
    float3 position;
    float aspect_ratio;
    float inv_focal_length;
    float focal_distance;
    float aperture_angle;
    int aperture_polygon;
    float aperture_radius;
} camera;

typedef struct
{
    float3 direction;
    float3 color;
    float cos_solid_angle;
} directional_light;

typedef struct
{
    bvh tlas;
    camera cam;
    /* The lighting setup is very minimal for now. There's only one directional
     * light and nothing more.
     */
    directional_light light;
} subframe;

#ifdef __cplusplus
#include <unordered_map>
#include <string>

struct scene
{
    mesh_buffers mesh_buf;
    bvh_buffers bvh_buf;

    /* Stores meshes & their BVH acceleration structures by their name.
     * This is not indexed during rendering, it's only used to organize the scene
     * in setup_animation_frame().
     */
    std::unordered_map<std::string, std::pair<mesh, bvh>> meshes;

    /* The same mesh is allowed to appear multiple times in the scene, you can
     * just add multiple instances of it.
     */
    std::vector<tlas_instance> instances;

    /* Used internally by the renderer to avoid having to recreate the world
     * on each frame.
     */
    uint static_instance_count = 0;

    /* Many things can change during the frame. These are stored in the
     * subframes.
     */
    std::vector<subframe> subframes;
};

/* This program plays a hardcoded animation, so there are no parameters here. */
scene load_scene();
void setup_animation_frame(scene& s, uint frame_index);
uint get_animation_frame_count(const scene& s);
#endif

#endif
