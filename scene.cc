#include "scene.hh"
#include "ray_query.hh"
#include <algorithm>
#define OBJECT_COUNT 1024

struct gradient_stop
{
    float t;
    float4 value;
};

float4 get_gradient_value(const std::vector<gradient_stop>& gradient, float t)
{
    auto it = std::lower_bound(
        gradient.begin(), gradient.end(), t,
        [](const gradient_stop& stop, float t) { return stop.t < t; }
    );

    if(it == gradient.begin()) return gradient.front().value;
    if(it == gradient.end()) return gradient.back().value;
    return mix((it-1)->value, it->value, (t - (it-1)->t)/(it->t - (it-1)->t));
}

struct animation_stop
{
    float start;
    float duration; // Time taken to transition to 'value'.
    float from;
    float to;
    float* variable;
};

void play_animation_track(size_t stop_count, const animation_stop* anim, float t)
{
    while(anim->start <= t && stop_count > 0)
    {
        float lt = anim->duration == 0 ? 1.0f : std::clamp((t - anim->start) / anim->duration, 0.0f, 1.0f);
        *anim->variable = mix(anim->from, anim->to, lt);
        stop_count--;
        anim++;
    }
}

std::pair<mesh, bvh>& load_mesh_bvh_pair(scene& s, const char* name, const char* obj_file)
{
    mesh m = load_mesh(s.mesh_buf, obj_file);
    bvh blas = build_blas(m, s.mesh_buf, s.bvh_buf);
    return s.meshes[name] = {m, blas};
}

void add_instance(scene& s, const char* name, mat4 transform)
{
    const auto& pair = s.meshes.at(name);
    s.instances.push_back(tlas_instance{
        pair.second,
        pair.first,
        transform,
        inverse4(transform)
    });
}

void add_instance(
    scene& s,
    const char* name,
    float3 pos = {0,0,0},
    float3 pitch_yaw_roll = {0,0,0},
    float3 scale = {1,1,1}
){
    mat4 transform = scaling(scale);
    transform = mul_m4m4(rotation_euler(pitch_yaw_roll * M_PI / 180.0f), transform);
    transform = mul_m4m4(translation(pos), transform);
    add_instance(s, name, transform);
}

std::vector<std::pair<const tlas_instance*, uint>> pull_instance_list(
    scene& s,
    uint static_begin,
    uint static_end,
    uint dynamic_begin,
    uint dynamic_end
){
    std::vector<std::pair<const tlas_instance*, uint>> instances;

    for(uint i = static_begin; i < static_end; ++i)
        instances.push_back({&s.instances[i], i});

    for(uint i = dynamic_begin; i < dynamic_end; ++i)
        instances.push_back({&s.instances[i], i});

    return instances;
}

bool terrain_trace(
    scene& s,
    bvh terrain_tlas,
    float3 origin,
    float3 dir,
    float3* hit_pos,
    float3* hit_normal
){
    ray_query rq = ray_query_initialize(
        terrain_tlas,
        s.instances.data(),
        s.bvh_buf.nodes.data(),
        s.bvh_buf.links.data(),
        s.mesh_buf.indices.data(),
        s.mesh_buf.pos.data(),
        origin, dir, 0, 1e9
    );
    while(ray_query_proceed(&rq))
        ray_query_confirm(&rq);
    if(rq.closest.thit < 0) return false;

    mesh m = s.instances[rq.closest.instance_id].m;
    uint tri_offset = m.index_offset + rq.closest.primitive_id * 3;
    uint i0 = s.mesh_buf.indices[tri_offset + 0];

    // Check if it's a water triangle, those don't count.
    if(s.mesh_buf.material[m.base_vertex_offset + i0].z != 0)
        return false;

    uint i1 = s.mesh_buf.indices[tri_offset + 1];
    uint i2 = s.mesh_buf.indices[tri_offset + 2];
    float3 n0 = s.mesh_buf.normal[m.base_vertex_offset + i0];
    float3 n1 = s.mesh_buf.normal[m.base_vertex_offset + i1];
    float3 n2 = s.mesh_buf.normal[m.base_vertex_offset + i2];

    float3 bary = rq.closest.barycentrics;
    *hit_normal = normalize(n0 * bary.x + n1 * bary.y + n2 * bary.z);
    *hit_pos = origin + dir * rq.closest.thit;

    return true;
}

scene load_scene()
{
    scene s;

    auto [terrain_mesh, terrain_bvh] = load_mesh_bvh_pair(s, "terrain", "data/terrain.obj");

    const std::vector<gradient_stop> albedo_gradient = {
        {-10, {0.25, 0.2, 0.1, 1}},
        {5, {0.2, 0.3, 0.02, 1}},
        {10, {0.2, 0.3, 0.02, 1}},
        {25, {0.3, 0.2, 0.1, 1}},
        {28, {0.95, 0.95, 0.95, 1}}
    };
    const std::vector<gradient_stop> material_gradient = {
        {5, {1.0, 0, 0, 0}},
        {25, {0.5, 0, 0, 0}},
        {28, {0.2, 0, 0, 0}}
    };
    
    // Update terrain mesh materials.
    for(uint i = 0; i < terrain_mesh.vertex_count; ++i)
    {
        float4& albedo = s.mesh_buf.albedo[i];
        float4& material = s.mesh_buf.material[i];
        if(material.z != 0) continue; // Don't update water, it's fine.
        float height = s.mesh_buf.pos[i].y;
        albedo = get_gradient_value(albedo_gradient, height);
        material = get_gradient_value(material_gradient, height);
    }
    load_mesh_bvh_pair(s, "leaf_tree", "data/leaf_tree.obj");
    load_mesh_bvh_pair(s, "maple_tree", "data/maple_tree.obj");
    load_mesh_bvh_pair(s, "pine_tree", "data/pine_tree.obj");
    load_mesh_bvh_pair(s, "tropical_tree", "data/tropical_tree.obj");
    load_mesh_bvh_pair(s, "willow_tree", "data/willow_tree.obj");
    load_mesh_bvh_pair(s, "rock0", "data/rock0.obj");
    load_mesh_bvh_pair(s, "rock1", "data/rock1.obj");
    load_mesh_bvh_pair(s, "rock2", "data/rock2.obj");
    load_mesh_bvh_pair(s, "rock3", "data/rock3.obj");
    load_mesh_bvh_pair(s, "rock4", "data/rock4.obj");

    load_mesh_bvh_pair(s, "armadillo", "data/armadillo.obj");
    load_mesh_bvh_pair(s, "buddha", "data/buddha.obj");
    load_mesh_bvh_pair(s, "bunny", "data/bunny.obj");
    load_mesh_bvh_pair(s, "dragon", "data/dragon.obj");
    load_mesh_bvh_pair(s, "teapot", "data/teapot.obj");

    load_mesh_bvh_pair(s, "end", "data/end.obj");
    load_mesh_bvh_pair(s, "logo", "data/logo.obj");

    add_instance(s, "terrain");

    // Build throwaway terrian TLAS. We just need it for the ray queries for
    // placing the various objects.
    std::pair<const tlas_instance*, uint> terrain_instance = {&s.instances[0], 0};
    bvh terrain_tlas = build_tlas(1, &terrain_instance, s.bvh_buf, s.bvh_buf);

    uint4 seed{1,2,3,4};
    for(int i = 0; i < OBJECT_COUNT; ++i)
    {
        float3 hit_pos;
        float3 hit_normal;
        float4 u = generate_uniform_random4(&seed);
        bool hit = terrain_trace(
            s,
            terrain_tlas,
            float3{u.x*200-100,200,u.y*200-100},
            float3{0,-1,0},
            &hit_pos,
            &hit_normal
        );
        // Don't place stuff on water.
        if(!hit)
            continue;

        // Only allow trees on fairly flat surfaces.
        bool tree_allowed = hit_normal.y > 0.7;
        // Rocks can be on more tilted surfaces, but not vertical.
        bool rock_allowed = hit_normal.y > 0.9;

        if(!tree_allowed && !rock_allowed)
            continue;

        float tree_probability = 0.3f;

        int spawn_type = 0;
        if(rock_allowed && !tree_allowed) spawn_type = 1;
        else if(!rock_allowed && tree_allowed) spawn_type = 0;
        else spawn_type = u.z < tree_probability ? 0 : 1;

        if(spawn_type == 0)
        {
            u.z /= tree_probability;
            // Spawn a tree.
            mat4 transform = rotation_euler({0,(float)(2.0f*M_PI*u.w),0});
            transform = mul_m4m4(translation(hit_pos), transform);
            if(hit_pos.y < 10)
                add_instance(s, "tropical_tree", transform);
            else if(hit_pos.y < 20)
            {
                if(u.z < 0.3)
                    add_instance(s, "maple_tree", transform);
                else if(u.z < 0.3)
                    add_instance(s, "leaf_tree", transform);
                else
                    add_instance(s, "willow_tree", transform);
            }
            else
                add_instance(s, "pine_tree", transform);
        }
        else
        {
            u.z = (u.z-tree_probability) / (1-tree_probability);
            // Spawn a rock.
            mat4 transform = expand_m3m4(create_tangent_space(hit_normal));
            std::swap(transform.r[2], transform.r[1]);
            transform = mul_m4m4(translation(hit_pos), transform);
            if(!tree_allowed)
            {
                if(u.z < 0.6) add_instance(s, "rock3", transform);
                else add_instance(s, "rock4", transform);
            }
            else
            {
                if(u.z < 0.3) add_instance(s, "rock0", transform);
                else if(u.z < 0.3) add_instance(s, "rock1", transform);
                else add_instance(s, "rock2", transform);
            }
        }
    }

    pop_bvh(s.bvh_buf, terrain_tlas);
    s.static_instance_count = s.instances.size();

    return s;
}

void setup_animation_frame(scene& s, uint frame_index)
{
    // Clear previous frame
    if(s.subframes.size() != 0)
        pop_bvh(s.bvh_buf, s.subframes[0].tlas);
    s.instances.resize(s.static_instance_count);
    s.subframes.clear();

    const float3 camera_start_pos = float3{-81.4,65,-113.6};
    const float3 camera_start_ori = float3{30.6,146.6,0};

    camera cam;
    cam.position = camera_start_pos;
    cam.aspect_ratio = IMAGE_WIDTH/float(IMAGE_HEIGHT);
    float fov = 80.0f;
    float3 cam_orientation = camera_start_ori;

    cam.focal_distance = 2.0f;
    cam.aperture_angle = M_PI/16.0f;
    cam.aperture_polygon = 6;
    cam.aperture_radius = 0.0f;

    directional_light light;
    light.color = float3{4,4,4};
    light.cos_solid_angle = cos(4.0f * M_PI / 180.0f);
    light.direction = normalize(float3{0,1,1});

    float logo_visible = 0;
    float armadillo_visible = 0;
    float dragon_visible = 0;
    float bunny_visible = 0;
    float end_visible = 0;
    float3 logo_pos = camera_start_pos;

    float3 teapot_pos = float3{
        40.1, 13.95, 13.611633
    };
    float3 teapot_ori = float3{0,0,0};
    float3 armadillo_pos = float3{0,0,0};
    float3 armadillo_ori = float3{0,0,0};
    float3 dragon_pos = float3{0,0,0};
    float3 dragon_ori = float3{0,0,0};
    float3 bunny_pos = float3{0,0,0};
    float3 bunny_ori = float3{0,0,0};
    float3 end_pos = float3{0,0,0};
    float3 end_ori = float3{0,0,0};

    float anim_t = float(frame_index) / FRAMERATE * 30.0f;
    const animation_stop anim[] = {
        {0, 120, 1, 0, &logo_visible},
        {60, 60, camera_start_pos.x, -90.6, &cam.position.x},
        {60, 60, camera_start_pos.y, 55, &cam.position.y},
        {60, 60, camera_start_pos.z, -67.8, &cam.position.z},
        {60, 60, camera_start_ori.x, 42.6, &cam_orientation.x},
        {60, 60, camera_start_ori.y, 123.8, &cam_orientation.y},

        // Wild teapot zoom!
        {140, 0, 0, -11.6, &cam.position.x},
        {140, 0, 0, 14.3, &cam.position.y},
        {140, 0, 0, 60.6, &cam.position.z},
        {140, 0, 0, 11.4, &cam_orientation.x},
        {140, 0, 0, 133, &cam_orientation.y},
        {150, 10, 11.4, 0.6, &cam_orientation.x},
        {150, 10, 133, 50, &cam_orientation.y},
        {150, 160, 40.1, 47, &teapot_pos.x},
        {150, 160, 13.95, 13, &teapot_pos.y},
        {150, 160, 13.6, 29, &teapot_pos.z},
        {150, 10, 150, 210, &teapot_ori.y},
        {160, 10, 210, 150, &teapot_ori.y},
        {170, 10, 150, 210, &teapot_ori.y},
        {170, 60, 80, 10, &fov},
        {180, 10, 210, 150, &teapot_ori.y},
        {190, 10, 150, 210, &teapot_ori.y},
        {200, 10, 210, 150, &teapot_ori.y},
        {210, 10, 150, 210, &teapot_ori.y},
        {220, 10, 210, 150, &teapot_ori.y},
        {230, 10, 150, 210, &teapot_ori.y},
        {240, 10, 210, 150, &teapot_ori.y},
        {250, 10, 150, 210, &teapot_ori.y},
        {260, 10, 210, 150, &teapot_ori.y},
        {270, 10, 150, 210, &teapot_ori.y},
        {280, 10, 210, 150, &teapot_ori.y},
        {290, 10, 150, 210, &teapot_ori.y},

        // Teapot eating!
        {300, 0, 0, 60, &fov},
        {300, 0, 0, 8.0f, &cam.focal_distance},
        {300, 0, 0, 0.2f, &cam.aperture_radius},
        {300, 0, 0, 38.5, &cam.position.x},
        {300, 0, 0, 19.2, &cam.position.y},
        {300, 0, 0, 37.7, &cam.position.z},
        {300, 0, 0, 35.2, &cam_orientation.x},
        {300, 0, 0, 108.8, &cam_orientation.y},
        {300, 0, 0, 45.3, &teapot_pos.x},
        {300, 0, 0, 12.4, &teapot_pos.y},
        {300, 0, 0, 40.9, &teapot_pos.z},
        {300, 0, 0, 120, &teapot_ori.y},
        {300, 10, 10, 30, &teapot_ori.x},
        {310, 10, 30, 10, &teapot_ori.x},
        {320, 10, 10, 30, &teapot_ori.x},
        {330, 10, 30, 10, &teapot_ori.x},
        {340, 10, 10, 30, &teapot_ori.x},
        {350, 10, 30, 0, &teapot_ori.x},
        {370, 3, 120, 210, &teapot_ori.y},

        // Predator armadillo appears!
        {370, 0, 0, 1, &armadillo_visible},
        {370, 0, 0, 29.6, &armadillo_pos.x},
        {370, 0, 0, 9, &armadillo_pos.y},
        {370, 0, 0, 52.2, &armadillo_pos.z},
        {370, 0, 0, 65, &armadillo_ori.y},
        {375, 5, 35.2, 23.6, &cam_orientation.x},
        {375, 5, 108.8, 205.8, &cam_orientation.y},
        {375, 5, 60, 50, &fov},
        {380, 5, 8.0f, 16.0f, &cam.focal_distance},

        {380, 30, 29.6, 34.6, &armadillo_pos.x},
        {380, 30, 9, 11, &armadillo_pos.y},
        {380, 30, 52.2, 55.5, &armadillo_pos.z},
        {380, 30, 65, 30, &armadillo_ori.y},

        {420, 10, 34.6, 40, &armadillo_pos.x},
        {420, 10, 11, 11.9, &armadillo_pos.y},
        {420, 10, 55.5, 48.7, &armadillo_pos.z},
        {420, 10, 30, 10, &armadillo_ori.y},

        // Cut to zoom from behind Buddha statue
        {430, 0, 0, -43.14, &cam.position.x},
        {430, 0, 0, 34.1, &cam.position.y},
        {430, 0, 0, 45.6, &cam.position.z},
        {430, 0, 0, 13, &cam_orientation.x},
        {430, 0, 0, 90, &cam_orientation.y},
        {430, 0, 0, 0, &cam.aperture_radius},
        {430, 0, 0, 10, &fov},
        {450, 30, 10, 60, &fov},

        // Rotate around statue
        {490, 20, -43.14, -39, &cam.position.x},
        {490, 20, 34.1, 34, &cam.position.y},
        {490, 20, 45.6, 46.3, &cam.position.z},
        {490, 20, 13, 16.6, &cam_orientation.x},
        {490, 20, 90, -4, &cam_orientation.y},

        {510, 30, -39, -35.5, &cam.position.x},
        {510, 30, 34, 33.7, &cam.position.y},
        {510, 30, 46.3, 42.8, &cam.position.z},
        {510, 30, 16.6, 16, &cam_orientation.x},
        {510, 30, -4, -48.4, &cam_orientation.y},

        {540, 30, -35.5, -34.8, &cam.position.x},
        {540, 30, 33.7, 33.7, &cam.position.y},
        {540, 30, 42.8, 38.8, &cam.position.z},
        {540, 30, 16, 13.4, &cam_orientation.x},
        {540, 30, -48.4, -109.4, &cam_orientation.y},

        {570, 30, -34.8, -36.4, &cam.position.x},
        {570, 30, 33.7, 33.7, &cam.position.y},
        {570, 30, 38.8, 36.7, &cam.position.z},
        {570, 30, 13.4, 14, &cam_orientation.x},
        {570, 30, -109.4, -138, &cam_orientation.y},

        {600, 30, -36.4, -40.2, &cam.position.x},
        {600, 30, 33.7, 29.6, &cam.position.y},
        {600, 30, 36.7, 35.5, &cam.position.z},
        {600, 30, 14, -29.8, &cam_orientation.x},
        {600, 30, -138, -185.8, &cam_orientation.y},

        {630, 30, -40.2, -43.1, &cam.position.x},
        {630, 30, 29.6, 32, &cam.position.y},
        {630, 30, 35.5, 37.4, &cam.position.z},
        {630, 30, -29.8, -5, &cam_orientation.x},
        {630, 30, -185.8, -230.4, &cam_orientation.y},

        // Fly to dragon
        {660, 0, 0, 1, &dragon_visible},
        {660, 0, 0, -92.9, &dragon_pos.x},
        {660, 0, 0, 0, &dragon_pos.y},
        {660, 0, 0, 76.9, &dragon_pos.z},
        {660, 0, 0, 60, &dragon_ori.y},

        {660, 30, -43.1, -43, &cam.position.x},
        {660, 30, 32, 30.6, &cam.position.y},
        {660, 30, 37.4, 44.8, &cam.position.z},
        {660, 30, -5, 25.4, &cam_orientation.x},
        {660, 30, -230.4, -150.2, &cam_orientation.y},

        {690, 30, -43, -67, &cam.position.x},
        {690, 30, 30.6, 18, &cam.position.y},
        {690, 30, 44.8, 62.6, &cam.position.z},
        {690, 30, 25.4, 34.2, &cam_orientation.x},
        {690, 30, -150.2, -105, &cam_orientation.y},

        {720, 30, -67, -79.2, &cam.position.x},
        {720, 30, 18, 7.7, &cam.position.y},
        {720, 30, 62.6, 69.5, &cam.position.z},
        {720, 30, 34.2, 21.6, &cam_orientation.x},
        {720, 30, -105, -118.8, &cam_orientation.y},

        // Observe dragon climbing to solid ground
        {770, 0, 0, -78.6, &cam.position.x},
        {770, 0, 0, 6.8, &cam.position.y},
        {770, 0, 0, 83, &cam.position.z},
        {770, 0, 0, 17.6, &cam_orientation.x},
        {770, 0, 0, -38.2, &cam_orientation.y},
        {770, 0, 0, 0.4f, &cam.aperture_radius},
        {770, 0, 0, 12.0f, &cam.focal_distance},

        {780, 60, -78.6, -76.4, &cam.position.x},
        {780, 60, 6.8, 8.5, &cam.position.y},
        {780, 60, 83, 80.3, &cam.position.z},
        {780, 60, 17.6, 22.6, &cam_orientation.x},
        {780, 60, -38.2, -48.2, &cam_orientation.y},

        {780, 60, -92.9, -84, &dragon_pos.x},
        {780, 60, 0, 3, &dragon_pos.y},
        {780, 60, 76.9, 70.3, &dragon_pos.z},
        {780, 60, 0, -38.8, &dragon_ori.x},

        // Observe dragon walking coastline
        {860, 60, -89.6, -97.7, &cam.position.x},
        {860, 60, 13.3, 14.3, &cam.position.y},
        {860, 60, 65.4, 52.2, &cam.position.z},
        {860, 60, 19.6, 22, &cam_orientation.x},
        {860, 60, 69.6, 84.6, &cam_orientation.y},
        {860, 0, 0, 16.0f, &cam.focal_distance},

        {860, 0, 0, 0, &dragon_ori.x},
        {860, 0, 0, 0, &dragon_ori.y},

        {860, 60, -77.3, -81.8, &dragon_pos.x},
        {860, 60, 7.89, 7.74,  &dragon_pos.y},
        {860, 60, 60.86, 49.6,  &dragon_pos.z},

        {920, 60, -97.7, -89, &cam.position.x},
        {920, 60, 14.3, 14.4, &cam.position.y},
        {920, 60, 52.2, 49.2, &cam.position.z},
        {920, 60, 22, 23, &cam_orientation.x},
        {920, 60, 84.6, 52.6, &cam_orientation.y},

        {920, 60, -81.8, -81.1, &dragon_pos.x},
        {920, 60, 7.74, 8.4, &dragon_pos.y},
        {920, 60, 49.6, 41.6, &dragon_pos.z},

        // Dragon sees bunny
        {980, 0, 0, 0.4f, &cam.aperture_radius},
        {980, 0, 0, 1.0f, &bunny_visible},
        {980, 0, 0, -27.9, &dragon_pos.x},
        {980, 0, 0, 22, &dragon_pos.y},
        {980, 0, 0, -43.8, &dragon_pos.z},
        {980, 0, 0, -34.5, &bunny_pos.x},
        {980, 0, 0, -30, &dragon_ori.y},
        {980, 0, 0, 19.1, &bunny_pos.y},
        {980, 0, 0, -52, &bunny_pos.z},
        {980, 0, 0, -21.3, &cam.position.x},
        {980, 0, 0, 29.1, &cam.position.y},
        {980, 0, 0, -45.2, &cam.position.z},
        {980, 0, 0, 31.8, &cam_orientation.x},
        {980, 0, 0, -63.6, &cam_orientation.y},
        {980, 0, 0, 40, &fov},
        {980, 30, 5.0f, 16.0f, &cam.focal_distance},

        // Bunny sees dragon
        {1050, 0, 0, 0.0f, &cam.aperture_radius},
        {1050, 0, 0, -36.1, &cam.position.x},
        {1050, 0, 0, 19.8, &cam.position.y},
        {1050, 0, 0, -59.1, &cam.position.z},
        {1050, 0, 0, -14.4, &cam_orientation.x},
        {1050, 0, 0, -198.4, &cam_orientation.y},

        {1070, 20, 0, 90, &bunny_ori.y},
        {1090, 5, 90, 180, &bunny_ori.y},
        {1095, 5, 19.1, 22, &bunny_pos.y},
        {1100, 5, 22, 19.1, &bunny_pos.y},
        {1105, 5, 180, 90, &bunny_ori.y},

        {1104, 5, -34.5, -25.5, &bunny_pos.x},
        {1104, 5, 0, 30, &dragon_ori.y},
        {1110, 5, -27.9, -27.6, &dragon_pos.x},
        {1110, 5, 22, 19.1, &dragon_pos.y},
        {1110, 5, -43.8, -54.4, &dragon_pos.z},

        // Bunny on the run
        {1115, 0, 0, -4.2, &cam.position.x},
        {1115, 0, 0, 10.6, &cam.position.y},
        {1115, 0, 0, -89.6, &cam.position.z},
        {1115, 0, 0, 1.4, &cam_orientation.x},
        {1115, 0, 0, 191.6, &cam_orientation.y},
        {1115, 0, 0, 90, &dragon_ori.y},
        {1115, 0, 0, 0, &dragon_visible},

        {1115, 20, -6.6, 1.8, &bunny_pos.x},
        {1115, 20, 8.2, 7.6, &bunny_pos.y},
        {1115, 20, -79.3, -78.6, &bunny_pos.z},

        {1145, 0, 0, 1, &dragon_visible},
        {1145, 20, -15.6, 4.5, &dragon_pos.x},
        {1145, 20, 8.2, 7.6, &dragon_pos.y},
        {1145, 20, -79.3, -78.6, &dragon_pos.z},

        // Bunny is on coastline, surroundings seems safe
        {1165, 0, 0, 43.1, &cam.position.x},
        {1165, 0, 0, 10.2, &cam.position.y},
        {1165, 0, 0, -90.1, &cam.position.z},
        {1165, 0, 0, 32.0, &cam_orientation.x},
        {1165, 0, 0, 180.2, &cam_orientation.y},
        {1165, 0, 0, 0, &dragon_visible},

        {1165, 0, 0, 42.7, &bunny_pos.x},
        {1165, 0, 0, 4.7, &bunny_pos.y},
        {1165, 0, 0, -83.6, &bunny_pos.z},

        {1200, 20, 32.0, -7.8, &cam_orientation.x},
        {1200, 20, 180.2, 161.2, &cam_orientation.y},

        {1260, 20, -7.8, -5, &cam_orientation.x},
        {1260, 20, 161.2, 238.4, &cam_orientation.y},

        {1300, 20, -5, 32.0, &cam_orientation.x},
        {1300, 20, 238.5, 180.2, &cam_orientation.y},

        // Sunset overview
        {1360, 0, 0, 15.7, &cam.position.x},
        {1360, 0, 0, 19.1, &cam.position.y},
        {1360, 0, 0, 75.5, &cam.position.z},
        {1360, 0, 0, 8.2, &cam_orientation.x},
        {1360, 0, 0, -1.8, &cam_orientation.y},

        // Bunny views sunset, with dragon's silhouette slowly sliding in
        {1580, 0, 0, 44.9, &bunny_pos.x},
        {1580, 0, 0, 2.6, &bunny_pos.y},
        {1580, 0, 0, -88.9, &bunny_pos.z},
        {1580, 0, 0, 60, &fov},

        {1580, 0, 0, 30.0, &cam.position.x},
        {1580, 0, 0, 9.4, &cam.position.y},
        {1580, 0, 0, -78.8, &cam.position.z},
        {1580, 0, 0, 9.2, &cam_orientation.x},
        {1580, 0, 0, 37.0, &cam_orientation.y},
        {1580, 0, 0, 1, &dragon_visible},

        {1690, 60, 24.5, 32.9, &dragon_pos.x},
        {1690, 60, 4.5, 3.9, &dragon_pos.y},
        {1690, 60, -85.9, -88.2, &dragon_pos.z},

        // Sun sets, screen gets darker, armadillo's eyes show up as well
        {1700, 60, 55.7, 55.1, &armadillo_pos.x},
        {1700, 60, 4.9, 4.7, &armadillo_pos.y},
        {1700, 60, -75.9, -82.6, &armadillo_pos.z},
        {1700, 0, 0, -90, &armadillo_ori.y},

        // Fin.
        {1740, 0, 0, 1, &end_visible},
        {1740, 0, 0, 33, &end_pos.x},
        {1740, 30, 12, 7.6, &end_pos.y},
        {1740, 0, 0, -83, &end_pos.z},
        {1740, 0, 0, 37.0, &end_ori.y}
    };
    play_animation_track(std::size(anim), anim, anim_t);

    uint static_begin = 0;

    // Fill in frame-static instances. These can depend on frame_index, but not
    // on subframe index.
    if(logo_visible != 0)
    {
        mat4 transform = rotation_euler(camera_start_ori * M_PI / 180.0f);
        float4 dir = float4{0,0,-3,0};
        float4 rot_dir = mul_v4m4(dir, transform);
        logo_pos -= float3{-1.3, 2, -2};
        transform = mul_m4m4(translation(logo_pos), transform);
        add_instance(s, "logo", transform);
    }

    add_instance(s, "buddha", float3{-39.255131, 30.395447, 40.472446});

    uint static_end = s.instances.size();

    uint subframe_count =
        (SAMPLES_PER_PIXEL + SAMPLES_PER_MOTION_BLUR_STEP-1)
        / SAMPLES_PER_MOTION_BLUR_STEP;

    struct subframe_entries
    {
        uint dynamic_begin;
        uint dynamic_end;
    };
    std::vector<subframe_entries> entries;
    uint max_node_count = 0;
    for(uint i = 0; i < subframe_count; ++i)
    {
        float anim_t = float(frame_index + float(i) / subframe_count) / FRAMERATE * 30.0f;
        play_animation_track(std::size(anim), anim, anim_t);

        uint dynamic_begin = s.instances.size();
        // Fill in dynamic instances.
        add_instance(s, "teapot", teapot_pos, teapot_ori);
        if(armadillo_visible != 0)
            add_instance(s, "armadillo", armadillo_pos, armadillo_ori);
        if(dragon_visible != 0)
            add_instance(s, "dragon", dragon_pos, dragon_ori);
        if(bunny_visible != 0)
            add_instance(s, "bunny", bunny_pos, bunny_ori);
        if(end_visible != 0)
            add_instance(s, "end", end_pos, end_ori);

        uint dynamic_end = s.instances.size();
        entries.push_back({dynamic_begin, dynamic_end});

        // Keep track of maximum number of needed TLAS nodes.
        max_node_count += 2 * ((static_end - static_begin) + (dynamic_end - dynamic_begin));

        subframe sf;

        // Setup camera
        sf.cam = cam;
        sf.cam.orientation =
            extract_m4m3(rotation_euler(cam_orientation * M_PI / 180.0f));
        sf.cam.inv_focal_length = tan(fov * M_PI / 360.0f);

        // Setup lights
        float sunset_t = anim_t / (30.0f * 60.0f) * 1.1f - 0.05f;
        light.direction = float3{0, sinf(sunset_t*M_PI), cosf(sunset_t*M_PI)};
        sf.light = light;

        s.subframes.push_back(sf);
    }

    std::vector<bvh_buffers> local_bufs(subframe_count);
    for(uint i = 0; i < subframe_count; ++i)
    {
        std::vector<std::pair<const tlas_instance*, uint>> instances = pull_instance_list(
            s,
            static_begin,
            static_end,
            entries[i].dynamic_begin,
            entries[i].dynamic_end
        );
        s.subframes[i].tlas = build_tlas(instances.size(), instances.data(), s.bvh_buf, local_bufs[i]);
    }

    for(uint i = 0; i < subframe_count; ++i)
    {
        s.subframes[i].tlas.node_offset = s.bvh_buf.nodes.size();
        s.bvh_buf.nodes.insert(s.bvh_buf.nodes.end(), local_bufs[i].nodes.begin(), local_bufs[i].nodes.end());
        s.bvh_buf.links.insert(s.bvh_buf.links.end(), local_bufs[i].links.begin(), local_bufs[i].links.end());
    }
}

uint get_animation_frame_count(const scene& s)
{
    // 1 minute
    return 60 * FRAMERATE;
}
