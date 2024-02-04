#ifndef PATH_TRACER_HH
#define PATH_TRACER_HH

#include "ray_query.hh"
#include "config.hh"
#include "scene.hh"

/*============================================================================*\
|  Sampling functions                                                          |
\*============================================================================*/

inline float sample_gaussian(float u, float sigma, float epsilon)
{
    float k = u * 2.0f - 1.0f;
    k = clamp(k, -(1.0f-epsilon), 1.0f-epsilon);
    return sigma * 1.41421356f * inv_erf(k);
}

inline float2 sample_gaussian_weighted_disk(float2 u, float sigma)
{
    float r = sqrt(u.x);
    float theta = 2.0f * (float)M_PI * u.y;
    r = sample_gaussian(r, sigma, 1e-6f);
    return r * (float2){(float)cos(theta), (float)sin(theta)};
}

inline float3 sample_cosine_hemisphere(float2 u)
{
    float r = sqrt(u.x);
    float theta = 2.0f * (float)M_PI * u.y;
    float2 d = r * (float2){(float)cos(theta), (float)sin(theta)};
    return (float3){d.x, d.y, (float)sqrt(fmax(0.0f, 1.0f - dot(d, d)))};
}

inline float cosine_hemisphere_pdf(float3 dir)
{
    return fmax(dir.z * (1.0f/(float)M_PI), 0.0f);
}

inline float3 sample_cone(float3 dir, float cos_theta_min, float2 u)
{
    float cos_theta = mix(1.0f, cos_theta_min, u.x);
    float sin_theta = sqrt(1.0f - cos_theta * cos_theta);
    float phi = u.y * 2.0f * (float)M_PI;
    return mul_m3v3(create_tangent_space(dir), (float3){
        (float)cos(phi) * sin_theta, (float)sin(phi) * sin_theta, cos_theta
    });
}

inline float2 sample_regular_polygon(float2 u, float angle, uint sides)
{
    float side = floor(u.x * sides);
    u.x *= sides;
    u.x = u.x - floor(u.x);
    float side_radians = (2.0f*(float)M_PI)/sides;
    float a1 = side_radians * side + angle;
    float a2 = side_radians * (side + 1.0f) + angle;
    float2 b = {(float)sin(a1), (float)cos(a1)};
    float2 c = {(float)sin(a2), (float)cos(a2)};
    u = u.x+u.y > 1.0f ? 1.0f - u : u;
    return b * u.x + c * u.y;
}

/* Derived from:
 * https://arxiv.org/pdf/2306.05044.pdf
 */
inline float3 sample_ggx_vndf(float3 view, float roughness, float2 u)
{
    if(roughness < 1e-3f)
        return (float3){0,0,1};

    float3 v = normalize((float3){roughness * view.x, roughness * view.y, view.z});

    float phi = 2.0f * (float)M_PI * u.x;

    float z = fma((1.0f - u.y), (1.0f + v.z), -v.z);
    float sin_theta = sqrt(clamp(1.0f - z * z, 0.0f, 1.0f));
    float x = sin_theta * cos(phi);
    float y = sin_theta * sin(phi);
    float3 h = (float3){x, y, z} + v;

    return normalize((float3){roughness * h.x, roughness * h.y, fmax(0.0f, h.z)});
}

/*============================================================================*\
|  Materials                                                                   |
\*============================================================================*/

inline float fresnel_schlick_bidir_attenuated(float v_dot_h, float f0, float eta, float roughness)
{
    if(eta > 1.0f)
    {
        float sin_theta2 = eta * eta * (1.0f - v_dot_h * v_dot_h);
        if(sin_theta2 >= 1.0f) return 1.0f;
        v_dot_h = sqrt(1.0f - sin_theta2);
    }
    return f0 + (fmax(1.0f - roughness, f0) - f0) * pow(fmax(1.0f - v_dot_h, 0.0f), 5.0f);
}

inline float fresnel_schlick_bidir(float v_dot_h, float f0, float eta)
{
    return fresnel_schlick_bidir_attenuated(v_dot_h, f0, eta, 0);
}

inline float trowbridge_reitz_distribution(float hdotn, float a)
{
    float a2 = a * a;
    float denom = hdotn * hdotn * (a2 - 1.0f) + 1.0f;
    return a2 / fmax((float)M_PI * denom * denom, 1e-10f);
}

inline float trowbridge_reitz_masking_shadowing(
    float ldotn, float ldoth,
    float vdotn, float vdoth,
    float a
){
    if(vdotn * vdoth < 0) return 0;
    if(ldotn * ldoth < 0) return 0;
    return 0.5f / (
        fabs(vdotn) * sqrt(ldotn * ldotn - a * a * ldotn * ldotn + a * a) +
        fabs(ldotn) * sqrt(vdotn * vdotn - a * a * vdotn * vdotn + a * a)
    );
}

inline float trowbridge_reitz_masking(float vdotn, float vdoth, float a)
{
    if(vdotn * vdoth < 0) return 0;
    return 2.0f * vdotn / (vdotn + sqrt(vdotn * vdotn * (1.0f - a * a) + a * a));
}

inline float3 bsdf_core(
    float3 light,
    float3 h,
    float3 view,
    float3 albedo,
    float roughness,
    float metallic,
    float transmission,
    float eta,
    float f0,
    float distribution,
    float* reflection_pdf,
    float* diffuse_pdf,
    float* transmission_pdf
){
    const bool brdf = light.z > 0;
    const float ldotn = light.z;
    const float vdotn = view.z;
    const float hdotn = h.z;
    const float vdoth = dot(view, h);
    const float ldoth = dot(light, h);

    float fresnel = fresnel_schlick_bidir(vdoth, f0, eta);

    float geometry = trowbridge_reitz_masking_shadowing(
        ldotn, ldoth, vdotn, vdoth, roughness
    );
    float G1 = trowbridge_reitz_masking(vdotn, vdoth, roughness);

    float3 color;
    if(brdf)
    { /* BRDF */
        color = (albedo * metallic + fresnel * (1.0f - metallic)) * geometry * distribution;
        color += (1.0f - fresnel) * (1.0f - metallic) * (1.0f - transmission) / (float)M_PI * albedo;

        *reflection_pdf = G1 * distribution / (4.0f * view.z);
        *diffuse_pdf = cosine_hemisphere_pdf(light);
        *transmission_pdf = 0;
    }
    else
    { /* BTDF */
        float denom = eta * vdoth + ldoth;
        color = albedo * (transmission * fabs(vdoth * ldoth) * (1.0f - fresnel) * 4.0f * geometry * distribution / (denom * denom));

        *reflection_pdf = 0;
        *diffuse_pdf = 0;
        *transmission_pdf = fabs(vdoth * ldoth) * G1 * distribution / (fabs(view.z) * denom * denom);
    }

    return color * fabs(ldotn);
}

/* Tangent-space BSDF */
inline float3 bsdf(
    float3 light,
    float3 view,
    float3 albedo,
    float roughness,
    float metallic,
    float transmission,
    float eta,
    float* out_pdf
){
    float3 h;
    if(light.z > 0) h = normalize(view + light);
    else h = sign(eta - 1.0f) * normalize(light + eta * view);
    float distribution = trowbridge_reitz_distribution(h.z, roughness);

    float f0 = (1.0f - eta)/(1.0f + eta);
    f0 *= f0;

    float reflection_prob = mix(
        1.0f, fresnel_schlick_bidir_attenuated(view.z, f0, eta, roughness),
        luminance(albedo) * (1.0f-metallic)
    );
    float transmission_prob = (1.0 - reflection_prob) * transmission;
    float diffuse_prob = (1.0 - reflection_prob) * (1.0f - transmission);

    float reflection_pdf;
    float diffuse_pdf;
    float transmission_pdf;
    float3 attenuation = bsdf_core(
        light, h, view, albedo, roughness, metallic, transmission, eta, f0,
        roughness < 1e-3f ? 0.0f : distribution, &reflection_pdf, &diffuse_pdf,
        &transmission_pdf
    );
    *out_pdf =
        reflection_pdf * reflection_prob +
        diffuse_pdf * diffuse_prob +
        transmission_pdf * transmission_prob;
    return attenuation;
}

inline void sample_bsdf(
    float3 u,
    float3 view,
    float3 albedo,
    float roughness,
    float metallic,
    float transmission,
    float eta,
    float3* out_dir,
    float3* out_attenuation,
    float* out_pdf
){
    float3 h = sample_ggx_vndf(view, roughness, (float2){u.x, u.y});

    float f0 = (1.0f - eta)/(1.0f + eta);
    f0 *= f0;

    float reflection_prob = mix(
        1.0f, fresnel_schlick_bidir_attenuated(view.z, f0, eta, roughness),
        luminance(albedo) * (1.0f-metallic)
    );
    float transmission_prob = (1.0 - reflection_prob) * transmission;
    float diffuse_prob = (1.0 - reflection_prob) * (1.0f - transmission);

    bool diffuse = false;
    bool bad = false;
    if((u.z -= reflection_prob) <= 0)
    { /* Reflection */
        *out_dir = reflect(-view, h);
        bad = out_dir->z <= 0;
    }
    else if((u.z -= transmission_prob) <= 0)
    { /* Refraction */
        *out_dir = refract(-view, h, eta);
        bad = out_dir->z >= 0;
    }
    else
    { /* Diffuse scattering */
        *out_dir = sample_cosine_hemisphere((float2){u.x, u.y});
        h = normalize(*out_dir + view);
        diffuse = true;
        bad = out_dir->z == 0;
    }

    if(bad)
    {
        *out_dir = (float3){0,0,1};
        *out_attenuation = (float3){0,0,0};
        *out_pdf = 1;
        return;
    }

    float distribution = trowbridge_reitz_distribution(h.z, roughness);
    if(roughness < 1e-3f)
        distribution = diffuse ? 0 : fabs(4.0f * out_dir->z * view.z);

    float reflection_pdf;
    float diffuse_pdf;
    float transmission_pdf;
    *out_attenuation = bsdf_core(
        *out_dir, h, view, albedo, roughness, metallic, transmission, eta, f0,
        distribution, &reflection_pdf, &diffuse_pdf, &transmission_pdf
    );
    *out_pdf =
        reflection_pdf * reflection_prob +
        transmission_pdf * transmission_prob;

    /* Mark extremities with negative PDFs. */
    if(roughness < 1e-3f && !diffuse)
        *out_pdf = -*out_pdf;
    else
        *out_pdf += diffuse_pdf * diffuse_prob;
}

/*============================================================================*\
|  Ray tracing helpers                                                         |
\*============================================================================*/

/* A lot of the path tracing functions need access to the scene data. The
 * runtime scene data is stored in this structure to avoid passing each
 * separately, as that's overly verbose.
 */
typedef struct
{
    /* Acceleration structures */
    bvh tlas;
    const tlas_instance* instances;
    const bvh_node* node_array;
    const bvh_link* link_array;
    const uint* mesh_indices;
    const float3* mesh_pos;
    const float3* mesh_normal;
    const float4* mesh_albedo;
    const float4* mesh_material;
    directional_light light;
} pt_context;

typedef struct
{
    float thit;
    float3 pos;
    mat3 tbn;

    float3 albedo;
    float alpha;
    float roughness;
    float metallic;
    float emission;
    float transmission;
    /* All transmissive things have the same IOR here... :) but the value
     * of eta depends on whether we're entering or exiting.
     */
    float eta;
    float nee_pdf;
} hit_info;

inline hit_info trace_ray(pt_context ctx, float3 origin, float3 dir, float tmin)
{
    ray_query rq = ray_query_initialize(
        ctx.tlas, ctx.instances, ctx.node_array, ctx.link_array,
        ctx.mesh_indices, ctx.mesh_pos, origin, dir, tmin, 1e9
    );
    while(ray_query_proceed(&rq))
    {
        ray_query_confirm(&rq);
    }

    hit_info hi;

    hi.thit = rq.closest.thit;
    hi.nee_pdf = 0;
    if(hi.thit < 0)
    {
        /* Missed, hit the sky. The actual sky is added as a volumetric pass, so
         * it's not explicitly here.
         */
        hi.albedo = (float3){0.0f, 0.0f, 0.0f};

        float visible = dot(ctx.light.direction, dir) > ctx.light.cos_solid_angle;
        hi.nee_pdf = visible / (2.0f * (float)M_PI * (1.0f - ctx.light.cos_solid_angle));
        hi.albedo += visible * ctx.light.color * (hi.nee_pdf == 0.0f ? 1.0f : hi.nee_pdf);
        hi.emission = 1.0f;
    }
    else
    { /* Hit a triangle. */
        hi.pos = origin + dir * rq.closest.thit;
        mesh m = ctx.instances[rq.closest.instance_id].m;
        mat3 rot = extract_m4m3(ctx.instances[rq.closest.instance_id].transform);

        uint tri_offset = m.index_offset + rq.closest.primitive_id * 3;
        uint i0 = ctx.mesh_indices[tri_offset + 0];
        uint i1 = ctx.mesh_indices[tri_offset + 1];
        uint i2 = ctx.mesh_indices[tri_offset + 2];

        float3 n0 = ctx.mesh_normal[m.base_vertex_offset + i0];
        float3 n1 = ctx.mesh_normal[m.base_vertex_offset + i1];
        float3 n2 = ctx.mesh_normal[m.base_vertex_offset + i2];
        float4 a0 = ctx.mesh_albedo[m.base_vertex_offset + i0];
        float4 a1 = ctx.mesh_albedo[m.base_vertex_offset + i1];
        float4 a2 = ctx.mesh_albedo[m.base_vertex_offset + i2];
        float4 m0 = ctx.mesh_material[m.base_vertex_offset + i0];
        float4 m1 = ctx.mesh_material[m.base_vertex_offset + i1];
        float4 m2 = ctx.mesh_material[m.base_vertex_offset + i2];

        float3 bary = rq.closest.barycentrics;
        float4 albedo = a0 * bary.x + a1 * bary.y + a2 * bary.z;
        float4 mat = m0 * bary.x + m1 * bary.y + m2 * bary.z;
        float3 n = n0 * bary.x + n1 * bary.y + n2 * bary.z;
        n = normalize(mul_m3v3(rot, n));

        float ior = 1.5f;
        if(rq.closest.back_face)
        {
            hi.eta = ior;
            n = -n;
        }
        else hi.eta = 1.0f/ior;

        hi.tbn = create_tangent_space(n);

        hi.albedo = (float3){albedo.x, albedo.y, albedo.z};
        hi.alpha = albedo.w;
        hi.roughness = mat.x * mat.x;
        hi.metallic = mat.y;
        hi.transmission = mat.z;
        hi.emission = mat.w;
    }
    return hi;
}

/* Shadowed if true. */
inline bool trace_shadow_ray(
    pt_context ctx,
    float3 origin,
    float3 dir,
    float tmin,
    float tmax
){
    ray_query rq = ray_query_initialize(
        ctx.tlas, ctx.instances, ctx.node_array, ctx.link_array,
        ctx.mesh_indices, ctx.mesh_pos, origin, dir, tmin, tmax
    );
    return ray_query_proceed(&rq);
}

inline void get_camera_ray(camera cam, float2 u, float2 coord, float3* dir, float3* origin)
{
    float2 uv = (float2){coord.x, coord.y} / (float2){IMAGE_WIDTH, IMAGE_HEIGHT} * 2.0f - 1.0f;
    uv.x *= cam.aspect_ratio;
    uv.y = -uv.y;

    float2 aperture_pos = (float2){0,0};
    if(cam.aperture_polygon > 3)
        aperture_pos = sample_regular_polygon(u, cam.aperture_angle, cam.aperture_polygon) * cam.aperture_radius;

    *origin = (float3){aperture_pos.x, aperture_pos.y, 0.0f};
    *dir = (float3){
        uv.x * cam.inv_focal_length,
        uv.y * cam.inv_focal_length,
        -1
    } * cam.focal_distance;
    *dir = normalize(*dir - *origin);

    /* Move to world space */
    *dir = mul_m3v3(cam.orientation, *dir);
    *origin = mul_m3v3(cam.orientation, *origin) + cam.position;
}

/*============================================================================*\
|  Sky ray marching                                                            |
\*============================================================================*/

inline float3 nishita_atmosphere_attenuation(
    float jitter,
    int iterations,
    float3 pos,
    float3 view,
    float tmax
){
    const float3 earth_origin = (float3){0, -EARTH_RADIUS, 0};
    float3 attenuation = (float3){1.0f, 1.0f, 1.0f};

    float tmin, atmax;
    bool hit = ray_sphere_intersection(
        pos, view, earth_origin, EARTH_RADIUS+ATMOSPHERE_HEIGHT, &tmin, &atmax);
    tmin = fmax(tmin, 0);
    tmax = fmin(atmax, tmax < 0 ? MAX_RAY_DIST : tmax);
    /* Somehow managed to miss atmosphere, so early exit. */
    if(!hit) return attenuation;

    float segment = (tmax - tmin) / iterations;
    float rayleigh_optical_depth = 0;
    float mie_optical_depth = 0;
    bool shadowed = false;

    for(int i = 0; i < iterations; ++i)
    {
        float t = segment * (jitter + i);
        float height = length(pos + t * view - earth_origin) - EARTH_RADIUS;
        rayleigh_optical_depth += exp(-height/ATMOSPHERE_RAYLEIGH_SCALE_HEIGHT);
        mie_optical_depth += exp(-height/ATMOSPHERE_MIE_SCALE_HEIGHT);
        if(height < 0) shadowed = true;
    }

    float3 tau =
        (ATMOSPHERE_RAYLEIGH_COEFFICIENT * rayleigh_optical_depth +
        ATMOSPHERE_MIE_COEFFICIENT * mie_optical_depth) * segment;
    attenuation.x = exp(-tau.x);
    attenuation.y = exp(-tau.y);
    attenuation.z = exp(-tau.z);
    if(shadowed) attenuation = (float3){0.0f, 0.0f, 0.0f};

    return attenuation;
}

inline void nishita_atmosphere_scattering(
    uint4* seed,
    pt_context ctx,
    float3 pos,
    float3 view,
    float tmax,
    float3* attenuation,
    float3* in_scatter
){
    const float3 earth_origin = (float3){0, -EARTH_RADIUS, 0};

    *attenuation = (float3){1.0f, 1.0f, 1.0f};
    *in_scatter = (float3){0.0f, 0.0f, 0.0f};
    /* Distance is so small that the atmospheric scattering is negligible. */
    if(tmax > 0 && tmax < 1e3f) return;

    float tmin, atmax;
    bool hit = ray_sphere_intersection(
        pos, view, earth_origin, EARTH_RADIUS+ATMOSPHERE_HEIGHT, &tmin, &atmax);
    tmin = fmax(tmin, 0);
    tmax = fmin(atmax, tmax < 0 ? MAX_RAY_DIST : tmax);
    /* Missed atmosphere. */
    if(!hit) return;

    float interval = tmax - tmin;
    float segment = interval / ATMOSPHERE_PRIMARY_ITERATIONS;
    float4 jitter = generate_uniform_random4(seed);

    float mu = dot(view, ctx.light.direction);
    float rayleigh_phase = 3.0f / (16.0f * (float)M_PI) * (1.0f + mu * mu);
    const float g = ATMOSPHERE_MIE_ANISOTROPY;
    float mie_phase = 3.0f / (8.0f * (float)M_PI) * (1.0f - g * g) * (1.0f + mu * mu) /
        ((2.0f + g * g) * pow(1.0f + g * g - 2.0f * g * mu, 1.5f));

    float rayleigh_optical_depth = 0;
    float mie_optical_depth = 0;
    float3 rayleigh_sum = {0,0,0};
    float3 mie_sum = {0,0,0};
    for(int i = 0; i < ATMOSPHERE_PRIMARY_ITERATIONS; ++i)
    {
        float t = segment * (jitter.x + i);
        float3 p = pos + t * view;

        ray_sphere_intersection(
            p, ctx.light.direction, earth_origin, EARTH_RADIUS+ATMOSPHERE_HEIGHT, &tmin, &tmax);
        float light_segment = (tmax - tmin) / ATMOSPHERE_SECONDARY_ITERATIONS;
        float light_rayleigh_optical_depth = 0;
        float light_mie_optical_depth = 0;
        bool shadowed = false;

        for(int j = 0; j < ATMOSPHERE_SECONDARY_ITERATIONS; ++j)
        {
            float t = light_segment * (jitter.y + j);
            float height = length(p + t * ctx.light.direction - earth_origin) - EARTH_RADIUS;
            light_rayleigh_optical_depth += exp(-height/ATMOSPHERE_RAYLEIGH_SCALE_HEIGHT);
            light_mie_optical_depth += exp(-height/ATMOSPHERE_MIE_SCALE_HEIGHT);
            if(height < 0) shadowed = true;
        }

        float height = fmax(length(p-earth_origin) - EARTH_RADIUS, 0.0f);
        float rayleigh_density = exp(-height/ATMOSPHERE_RAYLEIGH_SCALE_HEIGHT) * segment;
        float mie_density = exp(-height/ATMOSPHERE_MIE_SCALE_HEIGHT) * segment;

        rayleigh_optical_depth += rayleigh_density;
        mie_optical_depth += mie_density;

        float3 tau =
            ATMOSPHERE_RAYLEIGH_COEFFICIENT * (light_rayleigh_optical_depth * light_segment + rayleigh_optical_depth) +
            ATMOSPHERE_MIE_COEFFICIENT * (light_mie_optical_depth * light_segment + mie_optical_depth);

        float3 local_attenuation;
        local_attenuation.x = exp(-tau.x);
        local_attenuation.y = exp(-tau.y);
        local_attenuation.z = exp(-tau.z);
        if(shadowed) local_attenuation = (float3){0.0f, 0.0f, 0.0f};

        rayleigh_sum += local_attenuation * rayleigh_density;
        mie_sum += local_attenuation * mie_density;
    }
    float3 tau =
        ATMOSPHERE_RAYLEIGH_COEFFICIENT * rayleigh_optical_depth +
        ATMOSPHERE_MIE_COEFFICIENT * mie_optical_depth;

    attenuation->x = exp(-tau.x);
    attenuation->y = exp(-tau.y);
    attenuation->z = exp(-tau.z);
    *in_scatter =
        (rayleigh_sum * ATMOSPHERE_RAYLEIGH_COEFFICIENT * rayleigh_phase +
        mie_sum * ATMOSPHERE_MIE_COEFFICIENT * mie_phase) * ctx.light.color * 4.0f;
}

/*============================================================================*\
|  Core path tracer                                                            |
\*============================================================================*/

inline float3 nee_branch(uint4* seed, pt_context ctx, hit_info info, float3 tview)
{
    float4 u = generate_uniform_random4(seed);
    float3 light_dir = sample_cone(ctx.light.direction, ctx.light.cos_solid_angle, (float2){u.x, u.y});
    float nee_pdf = 1.0f / (2.0f * (float)M_PI * (1.0f - ctx.light.cos_solid_angle));

    float bsdf_pdf = 0;
    float3 color = bsdf(
        mul_v3m3(light_dir, info.tbn), tview,
        info.albedo, info.roughness, info.metallic, info.transmission,
        info.eta, &bsdf_pdf
    ) * nee_pdf * ctx.light.color;
    if((color.x == 0 && color.y == 0 && color.z == 0) || trace_shadow_ray(
        ctx, info.pos, light_dir,
        MIN_RAY_DIST, MAX_RAY_DIST
    )) return (float3){0,0,0};

    float mis_pdf = 1.0f;
    if(ctx.light.cos_solid_angle < 1.0f)
        mis_pdf = (nee_pdf * nee_pdf + bsdf_pdf * bsdf_pdf) / nee_pdf;

    color *= nishita_atmosphere_attenuation(
        u.w, ATMOSPHERE_PRIMARY_ITERATIONS, info.pos, light_dir, MAX_RAY_DIST
    );

    return color / mis_pdf;
}

/* Calculates one path traced sample.
 *
 * Parameters:
 *     xy: pixel coordinate
 *     sample_index: index of sample in this pixel & frame
 *     subframes: collection data that changes within a frame (for motion blur)
 *     instances: TLAS instances (from scene)
 *     node_array: BVH nodes (from bvh_buffers)
 *     link_array: BVH links (from bvh_buffers)
 *     mesh_indices: vertex indices for triangles of all meshes (from mesh_buffers)
 *     mesh_pos: position for vertices of all meshes (from mesh_buffers)
 *     mesh_normal: normal vectors for vertices of all meshes (from mesh_buffers)
 *     mesh_albedo: surface colors for vertices of all meshes (from mesh_buffers)
 *     mesh_material: surface materials for vertices of all meshes (from mesh_buffers)
 */
inline float3 path_trace_pixel(
    uint2 xy,
    int sample_index,

    const subframe* subframes,

    /* Acceleration structures */
    const tlas_instance* instances,
    const bvh_node* node_array,
    const bvh_link* link_array,

    /* Mesh buffers */
    const uint* mesh_indices,
    const float3* mesh_pos,
    const float3* mesh_normal,
    const float4* mesh_albedo,
    const float4* mesh_material
){
    uint subframe_index = sample_index < 0 ?
        0 : sample_index / SAMPLES_PER_MOTION_BLUR_STEP;
    subframe sf = subframes[subframe_index];

    uint4 seed = (uint4){xy.x, xy.y, (uint)sample_index, STUDENT_ID};
    pcg4d(&seed); /* Initialize seed. */

    float4 u = generate_uniform_random4(&seed);

    /* Determine camera ray. A slight offset is added to cause anti-aliasing. */
    float2 film_offset = sample_gaussian_weighted_disk((float2){u.x, u.y}, 0.4f) + 0.5f;
    float3 ray_dir;
    float3 ray_o;
    get_camera_ray(
        sf.cam, (float2){u.z, u.w},
        (float2){xy.x + film_offset.x, xy.y + film_offset.y}, &ray_dir, &ray_o
    );

    pt_context ctx;
    ctx.tlas = sf.tlas;
    ctx.instances = instances;
    ctx.node_array = node_array;
    ctx.link_array = link_array;
    ctx.mesh_indices = mesh_indices;
    ctx.mesh_pos = mesh_pos;
    ctx.mesh_normal = mesh_normal;
    ctx.mesh_albedo = mesh_albedo;
    ctx.mesh_material = mesh_material;
    ctx.light = sf.light;

    /* Trace primary ray */
    hit_info info = trace_ray(ctx, ray_o, ray_dir, 0.0f);
    float3 attenuation = (float3){1,1,1};
    float3 contribution = (float3){0,0,0};

    float3 in_scatter;
    nishita_atmosphere_scattering(&seed, ctx, ray_o, ray_dir, info.thit, &attenuation, &in_scatter);

    contribution += in_scatter + attenuation * info.albedo * info.emission;

    /* Trace bounce rays. */
    float regularization = 1.0f;
    for(uint bounce = 0; bounce < MAX_BOUNCES && info.thit > 0; ++bounce)
    {
        /* Calculate tangent-space view ray. */
        float3 view = mul_v3m3(-ray_dir, info.tbn);
        if(view.z < 1e-7f) view = (float3){view.x, view.y, fmax(view.z, 1e-7f)};
        view = normalize(view);

        /* NEE ray */
        contribution += attenuation * nee_branch(&seed, ctx, info, view);

        /* BSDF bounce */
        float4 u = generate_uniform_random4(&seed);
        float3 tdir;
        float3 bsdf_attenuation;
        float bsdf_pdf;
        sample_bsdf(
            (float3){u.x, u.y, u.z}, view,
            info.albedo, info.roughness, info.metallic, info.transmission,
            info.eta, &tdir, &bsdf_attenuation, &bsdf_pdf
        );

        ray_dir = normalize(mul_m3v3(info.tbn, tdir));
        ray_o = info.pos;
        info = trace_ray(ctx, ray_o, ray_dir, MIN_RAY_DIST);

        float mis_pdf = bsdf_pdf < 0 ? -bsdf_pdf :
            (info.nee_pdf * info.nee_pdf + bsdf_pdf * bsdf_pdf) / bsdf_pdf;

        attenuation *= bsdf_attenuation;

        float3 atmosphere_attenuation;
        float3 in_scatter;
        nishita_atmosphere_scattering(&seed, ctx, ray_o, ray_dir, info.thit, &atmosphere_attenuation, &in_scatter);

        contribution += attenuation * (in_scatter + atmosphere_attenuation * info.albedo * info.emission) / mis_pdf;
        attenuation *= atmosphere_attenuation / fabs(bsdf_pdf);

        /* Path space regularization */
        if(bsdf_pdf > 0.0f)
            regularization *= fmax(1 - PATH_SPACE_REGULARIZATION_GAMMA / pow(bsdf_pdf, 0.25f), 0.0f);
        info.roughness = 1.0f - (1.0f - info.roughness) * regularization;
    }

    return contribution;
}

/*============================================================================*\
|  Post-processing                                                             |
\*============================================================================*/

/* Tonemaps one pixel.
 *
 * Parameters:
 *     color: averaged color output of path_trace_pixel
 * Returns the tonemapped color as an 8-bit BGRA color.
 */
inline uchar4 tonemap_pixel(float3 color)
{
    /* Simplistic ACES fit */
    color = (color * (2.51f * color + 0.03f)) / (color * (2.43f * color + 0.59f) + 0.14f);

    /* SRGB correction */
    color.x = color.x < 0.0031308f ? color.x * 12.92f : pow(color.x, 1.0f/2.4f) * 1.055f - 0.055f;
    color.y = color.y < 0.0031308f ? color.y * 12.92f : pow(color.y, 1.0f/2.4f) * 1.055f - 0.055f;
    color.z = color.z < 0.0031308f ? color.z * 12.92f : pow(color.z, 1.0f/2.4f) * 1.055f - 0.055f;

    color = clamp(color, (float3){0,0,0}, (float3){1,1,1});

    return (uchar4){
        (uchar)round(color.z*255.0f),
        (uchar)round(color.y*255.0f),
        (uchar)round(color.x*255.0f),
        255
    };
}

#endif
