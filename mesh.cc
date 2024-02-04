#include "mesh.hh"
#include <map>
#include <set>
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <memory>

struct mtl_material
{
    std::string name;
    float3 albedo = {1,1,1};
    float alpha = 0;
    float3 emission = {0,0,0};
    float roughness = 1;
    float metallicness = 0;
    float3 transmission = {0,0,0};
};

static std::unique_ptr<char[]> read_file(const char* path)
{
    FILE* f = fopen(path, "rb");

    if(!f)
    {
        fprintf(stderr, "Unable to open %s\n", path);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    size_t sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::unique_ptr<char[]> data(new char[sz+1]);
    if(fread(data.get(), 1, sz, f) != sz)
    {
        fclose(f);
        fprintf(stderr, "Unable to read %s\n", path);
        exit(1);
    }
    data[sz] = 0;
    fclose(f);
    return data;
}

static std::string read_string(char*& str)
{
    while(isspace(*str)) ++str;
    char* name = str;
    int name_len = 0;
    for(; !isspace(*str) && *str; ++str) ++name_len;
    return std::string(name, name_len);
}

static void load_mtl(
    std::vector<mtl_material>& materials,
    const char* mtl_file
){
    std::unique_ptr<char[]> data = read_file(mtl_file);

    char* str = data.get();
    mtl_material* mat = nullptr;
    while(*str)
    {
        while(isspace(*str)) ++str;

        char* command = str;
        int command_len = 0;

        for(; *str && !isspace(*str); ++str) ++command_len;

        if(strncmp(command, "newmtl", command_len) == 0)
        {
            std::string name_str = read_string(str);
            mtl_material tmp;
            tmp.name = name_str;
            materials.push_back(tmp);
            mat = &materials.back();
        }
        else if(!mat) {}
        else if(strncmp(command, "Kd", command_len) == 0)
        {
            mat->albedo.x = strtof(str, &str);
            mat->albedo.y = strtof(str, &str);
            mat->albedo.z = strtof(str, &str);
        }
        else if(strncmp(command, "Ke", command_len) == 0)
        {
            mat->emission.x = strtof(str, &str);
            mat->emission.y = strtof(str, &str);
            mat->emission.z = strtof(str, &str);
        }
        else if(strncmp(command, "d", command_len) == 0)
            mat->alpha = strtof(str, &str);
        else if(strncmp(command, "Pr", command_len) == 0)
            mat->roughness = strtof(str, &str);
        else if(strncmp(command, "Pm", command_len) == 0)
            mat->metallicness = strtof(str, &str);
        else if(strncmp(command, "Tf", command_len) == 0)
        {
            mat->transmission.x = strtof(str, &str);
            mat->transmission.y = strtof(str, &str);
            mat->transmission.z = strtof(str, &str);
        }
        while(*str && *str != '\n') str++;
    }
}

mesh load_mesh(
    mesh_buffers& mb,
    const char* obj_file
){
    mesh m;
    m.index_offset = mb.indices.size();
    m.base_vertex_offset = mb.pos.size();

    struct index_group
    {
        int pos_index = -1;
        int tex_index = -1;
        int normal_index = -1;
        int material_index = -1;

        bool operator<(const index_group& other) const
        {
            if(pos_index < other.pos_index) return true;
            else if(pos_index > other.pos_index) return false;
            else if(tex_index < other.tex_index) return true;
            else if(tex_index > other.tex_index) return false;
            else if(normal_index < other.normal_index) return true;
            else if(normal_index > other.normal_index) return false;
            else if(material_index < other.material_index) return true;
            else if(material_index > other.material_index) return false;
            return false;
        }
    };

    std::vector<float3> positions;
    std::vector<float2> texcoords;
    std::vector<float3> normals;
    std::vector<mtl_material> materials;
    std::vector<index_group> indices;

    std::string prefix(obj_file, strrchr(obj_file, '/')+1);

    int active_material_index = 0;
    materials.push_back(mtl_material{});

    std::unique_ptr<char[]> data = read_file(obj_file);

    char* str = data.get();
    while(*str)
    {
        while(isspace(*str)) ++str;

        char* command = str;
        int command_len = 0;

        for(; *str && !isspace(*str); ++str) ++command_len;

        if(strncmp(command, "v", command_len) == 0)
        {
            float3 pos;
            pos.x = strtof(str, &str);
            pos.y = strtof(str, &str);
            pos.z = strtof(str, &str);
            positions.push_back(pos);
        }
        else if(strncmp(command, "vn", command_len) == 0)
        {
            float3 normal;
            normal.x = strtof(str, &str);
            normal.y = strtof(str, &str);
            normal.z = strtof(str, &str);
            normals.push_back(normalize(normal));
        }
        else if(strncmp(command, "vt", command_len) == 0)
        {
            float2 uv;
            uv.x = strtof(str, &str);
            uv.y = strtof(str, &str);
            texcoords.push_back(uv);
        }
        else if(strncmp(command, "f", command_len) == 0)
        {
            for(int i = 0; i < 3; ++i)
            {
                index_group index;
                index.material_index = active_material_index;
                index.pos_index = strtol(str, &str, 0)-1;
                if(*str == '/') str++;
                index.tex_index = strtol(str, &str, 0)-1;
                if(*str == '/') str++;
                index.normal_index = strtol(str, &str, 0)-1;
                indices.push_back(index);
            }
        }
        else if(strncmp(command, "usemtl", command_len) == 0)
        {
            std::string name_str = read_string(str);
            for(size_t i = 0 ; i < materials.size(); ++i)
            {
                if(materials[i].name == name_str)
                {
                    active_material_index = i;
                    break;
                }
            }
        }
        else if(strncmp(command, "mtllib", command_len) == 0)
            load_mtl(materials, (prefix + read_string(str)).c_str());
        while(*str && *str != '\n') str++;
    }

    m.triangle_count = indices.size() / 3;
    m.vertex_count = 0;
    std::map<index_group, size_t> ig_to_index;
    for(uint i = 0; i < indices.size(); ++i)
    {
        index_group ig = indices[i];
        auto it = ig_to_index.find(ig);
        if(it == ig_to_index.end())
        {
            it = ig_to_index.insert({ig, ig_to_index.size()}).first;
            float3 pos{};
            if(ig.pos_index >= 0 && ig.pos_index < positions.size())
                pos = positions[ig.pos_index];
            float3 normal{};
            if(ig.normal_index >= 0 && ig.normal_index < normals.size())
                normal = normals[ig.normal_index];
            float4 albedo{};
            float4 material{};
            if(ig.material_index >= 0 && ig.material_index < materials.size())
            {
                const mtl_material& mat = materials[ig.material_index];
                albedo.x = mat.albedo.x;
                albedo.y = mat.albedo.y;
                albedo.z = mat.albedo.z;
                albedo.w = mat.alpha;
                material.x = mat.roughness;
                material.y = mat.metallicness;
                float3 scaled_emission = fmax(
                    mat.emission / fmax(mat.albedo, mat.emission),
                    float3{0,0,0}
                );
                if(mat.emission.x == 0) scaled_emission.x = 0;
                if(mat.emission.y == 0) scaled_emission.y = 0;
                if(mat.emission.z == 0) scaled_emission.z = 0;

                material.z = fmax(mat.transmission.x, fmax(mat.transmission.y, mat.transmission.z));
                material.w = fmax(scaled_emission.x, fmax(scaled_emission.y, scaled_emission.z));
            }

            mb.pos.push_back(pos);
            mb.normal.push_back(normal);
            mb.albedo.push_back(albedo);
            mb.material.push_back(material);
            m.vertex_count++;
        }
        mb.indices.push_back(it->second);
    }

    return m;
}
