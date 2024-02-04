#ifndef MATH_HH
#define MATH_HH

#ifdef __cplusplus
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

#include <cmath>

struct alignas(2) char2 { char x, y; };
struct alignas(4) char3 { char x, y, z; };
struct alignas(4) char4 { char x, y, z, w; };

struct alignas(2) uchar2 { uchar x, y; };
struct alignas(4) uchar3 { uchar x, y, z; };
struct alignas(4) uchar4 { uchar x, y, z, w; };

struct alignas(4) short2 { short x, y; };
struct alignas(8) short3 { short x, y, z; };
struct alignas(8) short4 { short x, y, z, w; };

struct alignas(4) ushort2 { ushort x, y; };
struct alignas(8) ushort3 { ushort x, y, z; };
struct alignas(8) ushort4 { ushort x, y, z, w; };

struct alignas(8) int2 { int x, y; };
struct alignas(16) int3 { int x, y, z; };
struct alignas(16) int4 { int x, y, z, w; };

struct alignas(8) uint2 { uint x, y; };
struct alignas(16) uint3 { uint x, y, z; };
struct alignas(16) uint4 { uint x, y, z, w; };

struct alignas(8) float2 { float x, y; };
struct alignas(16) float3 { float x, y, z; };
struct alignas(16) float4 { float x, y, z, w; };

#define ELEMENTWISE_OPERATORS X(+) X(-) X(*) X(/)
#define BITWISE_OPERATORS X(^) X(&) X(|) X(>>) X(<<)
#define ELEMENTWISE_ASSIGN_OPERATORS X(+=) X(-=) X(*=) X(/=)
#define BITWISE_ASSIGN_OPERATORS X(^=) X(&=) X(|=) X(>>=) X(<<=)

#define VECTOR_OPERATORS(type, op) \
inline type##2 operator op(type##2 a, type##2 b){return {type(a.x op b.x),type(a.y op b.y)};} \
inline type##3 operator op(type##3 a, type##3 b){return {type(a.x op b.x),type(a.y op b.y),type(a.z op b.z)};} \
inline type##4 operator op(type##4 a, type##4 b){return {type(a.x op b.x),type(a.y op b.y),type(a.z op b.z),type(a.w op b.w)};} \
inline type##2 operator op(type##2 a, type b){return {type(a.x op b),type(a.y op b)};} \
inline type##3 operator op(type##3 a, type b){return {type(a.x op b),type(a.y op b),type(a.z op b)};} \
inline type##4 operator op(type##4 a, type b){return {type(a.x op b),type(a.y op b),type(a.z op b),type(a.w op b)};} \
inline type##2 operator op(type a, type##2 b){return {type(a op b.x),type(a op b.y)};} \
inline type##3 operator op(type a, type##3 b){return {type(a op b.x),type(a op b.y),type(a op b.z)};} \
inline type##4 operator op(type a, type##4 b){return {type(a op b.x),type(a op b.y),type(a op b.z),type(a op b.w)};}

#define VECTOR_ASSIGN_OPERATORS(type, op) \
inline type##2& operator op(type##2& a, type##2 b){a.x op b.x;a.y op b.y;return a;} \
inline type##3& operator op(type##3& a, type##3 b){a.x op b.x;a.y op b.y;a.z op b.z;return a;} \
inline type##4& operator op(type##4& a, type##4 b){a.x op b.x;a.y op b.y;a.z op b.z;a.w op b.w;return a;} \
inline type##2& operator op(type##2& a, type b){a.x op b;a.y op b;return a;} \
inline type##3& operator op(type##3& a, type b){a.x op b;a.y op b;a.z op b;return a;} \
inline type##4& operator op(type##4& a, type b){a.x op b;a.y op b;a.z op b;a.w op b;return a;}

#define X(op) VECTOR_OPERATORS(float, op)
ELEMENTWISE_OPERATORS
#undef X

#define X(op) VECTOR_ASSIGN_OPERATORS(float, op)
ELEMENTWISE_ASSIGN_OPERATORS
#undef X

#define X(op) \
    VECTOR_OPERATORS(char, op) \
    VECTOR_OPERATORS(uchar, op) \
    VECTOR_OPERATORS(short, op) \
    VECTOR_OPERATORS(ushort, op) \
    VECTOR_OPERATORS(int, op) \
    VECTOR_OPERATORS(uint, op)
ELEMENTWISE_OPERATORS
BITWISE_OPERATORS
#undef X

#define X(op) \
    VECTOR_ASSIGN_OPERATORS(char, op) \
    VECTOR_ASSIGN_OPERATORS(uchar, op) \
    VECTOR_ASSIGN_OPERATORS(short, op) \
    VECTOR_ASSIGN_OPERATORS(ushort, op) \
    VECTOR_ASSIGN_OPERATORS(int, op) \
    VECTOR_ASSIGN_OPERATORS(uint, op)
ELEMENTWISE_ASSIGN_OPERATORS
BITWISE_ASSIGN_OPERATORS
#undef X

inline float dot(float2 a, float2 b){return a.x * b.x + a.y * b.y;}
inline float dot(float3 a, float3 b){return a.x * b.x + a.y * b.y + a.z * b.z;}
inline float dot(float4 a, float4 b){return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;}

inline float2 fabs(float2 a){return {float(fabs(a.x)), float(fabs(a.y))};}
inline float3 fabs(float3 a){return {float(fabs(a.x)), float(fabs(a.y)), float(fabs(a.z))};}
inline float4 fabs(float4 a){return {float(fabs(a.x)), float(fabs(a.y)), float(fabs(a.z)), float(fabs(a.w))};}

inline float2 operator-(float2 a){return {-a.x, -a.y};}
inline float3 operator-(float3 a){return {-a.x, -a.y, -a.z};}
inline float4 operator-(float4 a){return {-a.x, -a.y, -a.z, -a.w};}

inline float length(float2 a){return sqrt(dot(a, a));}
inline float length(float3 a){return sqrt(dot(a, a));}
inline float length(float4 a){return sqrt(dot(a, a));}

inline float2 normalize(float2 a){return a/length(a);}
inline float3 normalize(float3 a){return a/length(a);}
inline float4 normalize(float4 a){return a/length(a);}

inline float& pick(float2& a, int index){return ((float*)&a)[index];}
inline float& pick(float3& a, int index){return ((float*)&a)[index];}
inline float& pick(float4& a, int index){return ((float*)&a)[index];}
inline float pick(const float2& a, int index){return ((const float*)&a)[index];}
inline float pick(const float3& a, int index){return ((const float*)&a)[index];}
inline float pick(const float4& a, int index){return ((const float*)&a)[index];}

using std::fmax;
using std::fmin;
inline float3 fmax(float3 a, float3 b){return {fmax(a.x, b.x), fmax(a.y, b.y), fmax(a.z, b.z)};}
inline float3 fmin(float3 a, float3 b){return {fmin(a.x, b.x), fmin(a.y, b.y), fmin(a.z, b.z)};}

inline float3 cross(float3 a, float3 b){return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};}

inline float sign(float val)
{
    if(val < 0) return -1.0f;
    if(val > 0) return 1.0f;
    return val == -0.0f ? -0.0f : +0.0f;
}

inline float clamp(float val, float min, float max)
{ return fmin(fmax(val, min), max); }
inline float3 clamp(float3 val, float3 min, float3 max)
{
    return {
        fmin(fmax(val.x, min.x), max.x),
        fmin(fmax(val.y, min.y), max.y),
        fmin(fmax(val.z, min.z), max.z),
    };
}

inline float mix(float a, float b, float t) { return a * (1.0f-t) + b * t; }
inline float2 mix(float2 a, float2 b, float t) { return a * (1.0f-t) + b * t; }
inline float3 mix(float3 a, float3 b, float t) { return a * (1.0f-t) + b * t; }
inline float4 mix(float4 a, float4 b, float t) { return a * (1.0f-t) + b * t; }
#endif

typedef struct { float2 r[2]; } mat2;
typedef struct { float3 r[3]; } mat3;
typedef struct { float4 r[4]; } mat4;

inline mat2 transpose2(mat2 a){return (mat2){{{a.r[0].x, a.r[1].x}, {a.r[0].y, a.r[1].y}}};}
inline mat3 transpose3(mat3 a)
{
    return (mat3){{
        {a.r[0].x, a.r[1].x, a.r[2].x},
        {a.r[0].y, a.r[1].y, a.r[2].y},
        {a.r[0].z, a.r[1].z, a.r[2].z}
    }};
}
inline mat4 transpose4(mat4 a)
{
    return (mat4){{
        {a.r[0].x, a.r[1].x, a.r[2].x, a.r[3].x},
        {a.r[0].y, a.r[1].y, a.r[2].y, a.r[3].y},
        {a.r[0].z, a.r[1].z, a.r[2].z, a.r[3].z},
        {a.r[0].w, a.r[1].w, a.r[2].w, a.r[3].w}
    }};
}

inline mat2 mul_fm2(float b, mat2 a){return (mat2){{a.r[0] * b, a.r[1] * b}};}
inline mat3 mul_fm3(float b, mat3 a){return (mat3){{a.r[0] * b, a.r[1] * b, a.r[2] * b}};}
inline mat4 mul_fm4(float b, mat4 a){return (mat4){{a.r[0] * b, a.r[1] * b, a.r[2] * b, a.r[3] * b}};}

/* Adapted from GLM - https://github.com/g-truc/glm/blob/master/copying.txt */
inline mat4 inverse4(mat4 a)
{
    float c00 = a.r[2].z * a.r[3].w - a.r[3].z * a.r[2].w;
    float c02 = a.r[1].z * a.r[3].w - a.r[3].z * a.r[1].w;
    float c03 = a.r[1].z * a.r[2].w - a.r[2].z * a.r[1].w;
    float c04 = a.r[2].y * a.r[3].w - a.r[3].y * a.r[2].w;
    float c06 = a.r[1].y * a.r[3].w - a.r[3].y * a.r[1].w;
    float c07 = a.r[1].y * a.r[2].w - a.r[2].y * a.r[1].w;
    float c08 = a.r[2].y * a.r[3].z - a.r[3].y * a.r[2].z;
    float c10 = a.r[1].y * a.r[3].z - a.r[3].y * a.r[1].z;
    float c11 = a.r[1].y * a.r[2].z - a.r[2].y * a.r[1].z;
    float c12 = a.r[2].x * a.r[3].w - a.r[3].x * a.r[2].w;
    float c14 = a.r[1].x * a.r[3].w - a.r[3].x * a.r[1].w;
    float c15 = a.r[1].x * a.r[2].w - a.r[2].x * a.r[1].w;
    float c16 = a.r[2].x * a.r[3].z - a.r[3].x * a.r[2].z;
    float c18 = a.r[1].x * a.r[3].z - a.r[3].x * a.r[1].z;
    float c19 = a.r[1].x * a.r[2].z - a.r[2].x * a.r[1].z;
    float c20 = a.r[2].x * a.r[3].y - a.r[3].x * a.r[2].y;
    float c22 = a.r[1].x * a.r[3].y - a.r[3].x * a.r[1].y;
    float c23 = a.r[1].x * a.r[2].y - a.r[2].x * a.r[1].y;

    float4 f0 = {c00, c00, c02, c03};
    float4 f1 = {c04, c04, c06, c07};
    float4 f2 = {c08, c08, c10, c11};
    float4 f3 = {c12, c12, c14, c15};
    float4 f4 = {c16, c16, c18, c19};
    float4 f5 = {c20, c20, c22, c23};

    float4 v0 = {a.r[1].x, a.r[0].x, a.r[0].x, a.r[0].x};
    float4 v1 = {a.r[1].y, a.r[0].y, a.r[0].y, a.r[0].y};
    float4 v2 = {a.r[1].z, a.r[0].z, a.r[0].z, a.r[0].z};
    float4 v3 = {a.r[1].w, a.r[0].w, a.r[0].w, a.r[0].w};

    mat4 inv = {{
        (float4){v1 * f0 - v2 * f1 + v3 * f2} * (float4){+1, -1, +1, -1},
        (float4){v0 * f0 - v2 * f3 + v3 * f4} * (float4){-1, +1, -1, +1},
        (float4){v0 * f1 - v1 * f3 + v3 * f5} * (float4){+1, -1, +1, -1},
        (float4){v0 * f2 - v1 * f4 + v2 * f5} * (float4){-1, +1, -1, +1}
    }};

    float determinant = dot(a.r[0], (float4){inv.r[0].x, inv.r[1].x, inv.r[2].x, inv.r[3].x});
    return mul_fm4(1.0f / determinant, inv);
}

inline float2 mul_v2m2(float2 b, mat2 a){return (float2){dot(a.r[0], b), dot(a.r[1], b)};}
inline float3 mul_v3m3(float3 b, mat3 a){return (float3){dot(a.r[0], b), dot(a.r[1], b), dot(a.r[2], b)};}
inline float4 mul_v4m4(float4 b, mat4 a){return (float4){dot(a.r[0], b), dot(a.r[1], b), dot(a.r[2], b), dot(a.r[3], b)};}
inline float2 mul_m2v2(mat2 b, float2 a){return mul_v2m2(a, transpose2(b));}
inline float3 mul_m3v3(mat3 b, float3 a){return mul_v3m3(a, transpose3(b));}
inline float4 mul_m4v4(mat4 b, float4 a){return mul_v4m4(a, transpose4(b));}

inline mat2 mul_m2m2(mat2 b, mat2 a)
{
    mat2 bt = transpose2(b);
    return (mat2){{
        {dot(a.r[0], bt.r[0]), dot(a.r[0], bt.r[1])},
        {dot(a.r[1], bt.r[0]), dot(a.r[1], bt.r[1])}
    }};
}
inline mat3 mul_m3m3(mat3 b, mat3 a)
{
    mat3 bt = transpose3(b);
    return (mat3){{
        {dot(a.r[0], bt.r[0]), dot(a.r[0], bt.r[1]), dot(a.r[0], bt.r[2])},
        {dot(a.r[1], bt.r[0]), dot(a.r[1], bt.r[1]), dot(a.r[1], bt.r[2])},
        {dot(a.r[2], bt.r[0]), dot(a.r[2], bt.r[1]), dot(a.r[2], bt.r[2])}
    }};
}
inline mat4 mul_m4m4(mat4 b, mat4 a)
{
    mat4 bt = transpose4(b);
    return (mat4){{
        {dot(a.r[0], bt.r[0]), dot(a.r[0], bt.r[1]), dot(a.r[0], bt.r[2]), dot(a.r[0], bt.r[3])},
        {dot(a.r[1], bt.r[0]), dot(a.r[1], bt.r[1]), dot(a.r[1], bt.r[2]), dot(a.r[1], bt.r[3])},
        {dot(a.r[2], bt.r[0]), dot(a.r[2], bt.r[1]), dot(a.r[2], bt.r[2]), dot(a.r[2], bt.r[3])},
        {dot(a.r[3], bt.r[0]), dot(a.r[3], bt.r[1]), dot(a.r[3], bt.r[2]), dot(a.r[3], bt.r[3])}
    }};
}

inline mat2 add_m2m2(mat2 a, mat2 b) { return (mat2){{a.r[0]+b.r[0], a.r[1]+b.r[1]}}; }
inline mat3 add_m3m3(mat3 a, mat3 b)
{
    return (mat3){{a.r[0]+b.r[0], a.r[1]+b.r[1], a.r[2]+b.r[2]}};
}
inline mat4 add_m4m4(mat4 a, mat4 b)
{
    return (mat4){{a.r[0]+b.r[0], a.r[1]+b.r[1], a.r[2]+b.r[2], a.r[3]+b.r[3]}};
}

inline mat4 expand_m3m4(mat3 m)
{
    return (mat4){{
        {m.r[0].x, m.r[0].y, m.r[0].z, 0},
        {m.r[1].x, m.r[1].y, m.r[1].z, 0},
        {m.r[2].x, m.r[2].y, m.r[2].z, 0},
        {0,0,0,1},
    }};
}

inline mat3 extract_m4m3(mat4 m)
{
    return (mat3){{
        {m.r[0].x, m.r[0].y, m.r[0].z},
        {m.r[1].x, m.r[1].y, m.r[1].z},
        {m.r[2].x, m.r[2].y, m.r[2].z},
    }};
}

inline mat2 rotate2(float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return (mat2){{{c,-s}, {s,c}}};
}

inline mat4 rotation(float3 axis, float angle)
{
    float sa = sin(angle);
    float ca = cos(angle);
    mat3 K = {{{0,axis.z,-axis.y}, {-axis.z,0,axis.x}, {axis.y,-axis.x,0}}};
    mat3 R = {{{1,0,0}, {0,1,0}, {0,0,1}}};
    R = add_m3m3(R, mul_fm3(sa, K));
    R = add_m3m3(R, mul_fm3(1 - ca, mul_m3m3(K, K)));
    return expand_m3m4(R);
}

inline mat4 rotation_euler(float3 euler)
{
    float s_pitch = sin(euler.x);
    float c_pitch = cos(euler.x);
    float s_yaw = sin(euler.y);
    float c_yaw = cos(euler.y);
    float s_roll = sin(euler.z);
    float c_roll = cos(euler.z);

    mat3 pitch = {{{1,0,0}, {0,c_pitch,-s_pitch}, {0,s_pitch,c_pitch}}};
    mat3 yaw = {{{c_yaw, 0, s_yaw}, {0, 1, 0}, {-s_yaw, 0, c_yaw}}};
    mat3 roll = {{{c_roll, -s_roll, 0}, {s_roll, c_roll, 0}, {0, 0, 1}}};
    return expand_m3m4(mul_m3m3(roll, mul_m3m3(yaw, pitch)));
}

inline mat4 scaling(float3 scale)
{
    return (mat4){{
        {scale.x, 0, 0, 0},
        {0, scale.y, 0, 0},
        {0, 0, scale.z, 0},
        {0,0,0,1},
    }};
}

inline mat4 translation(float3 offset)
{
    return (mat4){{
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {offset.x, offset.y, offset.z, 1},
    }};
}

inline void ray_triangle_intersection_preprocess(float3 dir, int* axis, float3* S)
{
    float3 absdir = fabs(dir);
    float3 rdir = dir;
    *axis = 2;
    if(absdir.x > absdir.y && absdir.x > absdir.z)
    {
        *axis = 0;
        rdir = (float3){dir.z, dir.y, dir.x};
    }
    else if(absdir.y > absdir.z)
    {
        *axis = 1;
        rdir = (float3){dir.x, dir.z, dir.y};
    }
    *S = (float3){rdir.x, rdir.y, 1.0f} * (1.0f/rdir.z);
}

inline bool ray_triangle_intersection(
    float3 origin,
    int axis,
    float3 S,
    float3 pos0,
    float3 pos1,
    float3 pos2,
    float3* uvt,
    bool* back_face
){
    float3 A = pos0 - origin;
    float3 B = pos1 - origin;
    float3 C = pos2 - origin;

    float3 x = {A.x, B.x, C.x};
    float3 y = {A.y, B.y, C.y};
    float3 z = {A.z, B.z, C.z};

    if(axis == 0)
    {
        x = z;
        z = (float3){A.x, B.x, C.x};
    }
    else if(axis == 1)
    {
        y = z;
        z = (float3){A.y, B.y, C.y};
    }

    x -= S.x * z;
    y -= S.y * z;

    float3 uvw = cross(y, x);
    float det = uvw.x + uvw.y + uvw.z;
    *uvt = (float3){uvw.x, uvw.y, dot(uvw, S.z * z)} * (1.0f / det);
    *back_face = det < 0;
    if(S.z < 0) *back_face = !*back_face;
    if(axis != 2) *back_face = !*back_face;

    return
        det != 0.0f && uvt->z >= 0.0f &&
        ((uvw.x >= 0.0f && uvw.y >= 0.0f && uvw.z >= 0.0f) ||
         (uvw.x <= 0.0f && uvw.y <= 0.0f && uvw.z <= 0.0f));
}

/* dir must be an unit vector. */
inline bool ray_sphere_intersection(
    float3 origin, float3 dir, float3 pos, float radius,
    float* tmin, float* tmax
){
    float3 oc = origin - pos;
    float b = dot(oc, dir);
    float c = dot(oc, oc) - radius * radius;
    float discriminant = b * b - c;
    if(discriminant < 0) return false;
    discriminant = sqrt(discriminant);
    *tmin = -b - discriminant;
    *tmax = -b + discriminant;
    return true;
}

inline float3 create_tangent(float3 normal)
{
    float3 major;
    if(fabs(normal.x) < 0.57735026918962576451) major = (float3){1,0,0};
    else if(fabs(normal.y) < 0.57735026918962576451) major = (float3){0,1,0};
    else major = (float3){0,0,1};

    float3 tangent = normalize(cross(normal, major));
    return tangent;
}

inline mat3 create_tangent_space(float3 normal)
{
    float3 tangent = create_tangent(normal);
    float3 bitangent = cross(normal, tangent);
    return (mat3){{tangent, bitangent, normal}};
}

inline float luminance(float3 col)
{
    return dot(col, (float3){0.2126f, 0.7152f, 0.0722f});
}

inline float3 reflect(float3 I, float3 N)
{
    return I - 2.0f * dot(N, I) * N;
}

inline float3 refract(float3 I, float3 N, float eta)
{
    float ndoti = dot(N, I);
    float k = 1.0f - eta * eta * (1.0f - ndoti * ndoti);
    if(k < 0.0f) return (float3){0,0,0};
    else return eta * I - (eta * ndoti + sqrt(k)) * N;
}

inline float inv_erf(float x)
{
    float ln1x2 = log(1-x*x);
    const float a = 0.147f;
    const float p = 2.0f/((float)M_PI*a);
    float k = p + ln1x2 * 0.5f;
    float k2 = k*k;
    return sign(x) * sqrt(sqrt(k2-ln1x2*(1.0f/a))-k);
}

/* http://www.jcgt.org/published/0009/03/02/ */
inline uint4 pcg4d(uint4* seed)
{
    *seed = *seed * 1664525u + 1013904223u;
    *seed += (uint4){seed->y, seed->z, seed->x, seed->y} * (uint4){seed->w, seed->x, seed->y, seed->z};
    *seed ^= *seed >> 16u;
    *seed += (uint4){seed->y, seed->z, seed->x, seed->y} * (uint4){seed->w, seed->x, seed->y, seed->z};
    return *seed;
}

inline float4 generate_uniform_random4(uint4* seed)
{
    uint4 value = pcg4d(seed);
    float4 fvalue = {
        (float)value.x,
        (float)value.y,
        (float)value.z,
        (float)value.w
    };
    return fvalue * 2.3283064365386963e-10f;
}

#endif
