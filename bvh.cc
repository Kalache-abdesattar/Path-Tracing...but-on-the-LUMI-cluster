#include "bvh.hh"
#include <algorithm>
#include <cfloat>

struct aabb
{
    float3 min;
    float3 max;
};

struct leaf_node
{
    float3 min;
    float3 max;
    uint index;
};

struct build_node
{
    float3 min;
    float3 max;
    uint leaf_count;
    int axis;
    uint index;
    std::vector<build_node> children; // Leaf node if children.size() == 0
};

template<typename T>
void sort_nodes(T begin, T end, int axis)
{
    std::sort(begin, end,
        [&](const auto& a, const auto& b)
        {
            float a_centroid = pick(a.max, axis) + pick(a.min, axis);
            float b_centroid = pick(b.max, axis) + pick(b.min, axis);
            if(a_centroid < b_centroid) return true;
            else if(a_centroid > b_centroid) return false;
            else return a.index < b.index;
        }
    );
}

void build_recursive_sah(
    leaf_node* leaves,
    uint leaf_count,
    build_node& self
){
    self.axis = -1;
    self.leaf_count = leaf_count;

    if(leaf_count == 1)
    { // Only a leaf left.
        self.leaf_count = leaves[0].index;
        return;
    }

    float min_cost = FLT_MAX;
    int min_split = 0;
    aabb min_bounds0, min_bounds1;

    std::vector<aabb> first_bounds(leaf_count-1);
    std::vector<aabb> second_bounds(leaf_count-1);

    // Test splits on each axis
    for(int axis = 0; axis < 3; ++axis)
    {
        // Sort all nodes
        sort_nodes(leaves, leaves + leaf_count, axis);

        // Calculate potential bounding boxes for first side
        for(int i = 0; i < leaf_count-1; ++i)
        {
            first_bounds[i].min = i == 0 ? leaves[i].min :
                fmin(first_bounds[i-1].min, leaves[i].min);
            first_bounds[i].max = i == 0 ? leaves[i].max :
                fmax(first_bounds[i-1].max, leaves[i].max);

            int inv_i = leaf_count-1-i;
            second_bounds[inv_i-1].min = i == 0 ? leaves[inv_i].min :
                fmin(second_bounds[inv_i].min, leaves[inv_i].min);
            second_bounds[inv_i-1].max = i == 0 ? leaves[inv_i].max :
                fmax(second_bounds[inv_i].max, leaves[inv_i].max);
        }

        // Find split with minimum cost
        for(int i = 0; i < leaf_count-1; ++i)
        {
            aabb bounds0 = first_bounds[i];
            aabb bounds1 = second_bounds[i];
            float3 s0 = bounds0.max - bounds0.min;
            float3 s1 = bounds1.max - bounds1.min;

            float area0 = s0.x * s0.y + s0.z * s0.x + s0.y * s0.z;
            float area1 = s1.x * s1.y + s1.z * s1.x + s1.y * s1.z;

            float cost = (i+1) * area0 + (leaf_count-1-i) * area1;
            if(cost < min_cost)
            {
                min_bounds0 = bounds0;
                min_bounds1 = bounds1;
                min_cost = cost;
                min_split = i+1;
                self.axis = axis;
            }
        }
    }

    float3 size = self.max - self.min;
    min_cost /= size.x * size.y + size.z * size.x + size.y * size.z;
    // The constant cost of 2 for traversal appears more correct to me than
    // PBRTs 0.5.
    min_cost += 2.0f;
    if(leaf_count <= min_cost)
    {
        self.axis = 2;
        if(size.x > size.y && size.x > size.z) self.axis = 0;
        else if(size.y > size.z) self.axis = 1;
    }

    sort_nodes(leaves, leaves + leaf_count, self.axis);

    if(leaf_count <= min_cost)
    {
        // Looks like there's nothing to gain by splitting further, so we just
        // add the rest as leaves.
        for(uint i = 0; i < leaf_count; ++i)
            self.children.emplace_back(build_node{
                leaves[i].min, leaves[i].max, leaves[i].index, -1, {}
            });
    }
    else
    {
        // There's a benefit to splitting, so the tree gets deeper again.
        self.children = {
            {min_bounds0.min, min_bounds0.max},
            {min_bounds1.min, min_bounds1.max}
        };
        build_recursive_sah(leaves, min_split, self.children[0]);
        build_recursive_sah(
            leaves+min_split, leaf_count-min_split, self.children[1]
        );
    }
}

void traverse_bvh_nodes_bfs(
    build_node& branch,
    uint& node_index,
    std::vector<bvh_node>& out
){
    std::vector<build_node*> layer{&branch};
    std::vector<build_node*> next_layer;

    while(layer.size() != 0)
    {
        for(build_node* node: layer)
        {
            out.push_back({
                node->min.x, node->min.y, node->min.z,
                node->max.x, node->max.y, node->max.z
            });
            node->index = node_index++;
            for(build_node& child: node->children)
                next_layer.push_back(&child);
        }
        std::swap(layer, next_layer);
        next_layer.clear();
    }
}

void save_traversal_links(
    bool signs[3],
    const build_node& branch,
    uint cancel,
    bvh_link* links
){
    if(branch.children.size() == 0) // Leaf!
        links[branch.index] = {0x80000000u | branch.leaf_count, cancel};
    else
    { // Interior node!
        bool reverse = !signs[branch.axis];
        for(uint i = 0; i < branch.children.size(); ++i)
        {
            int inv_i = branch.children.size()-1-i;
            const build_node& child = branch.children[reverse ? inv_i : i];
            if(i == 0) links[branch.index] = {child.index, cancel};

            uint next_index = cancel;
            if(i < branch.children.size()-1)
                next_index = branch.children[reverse ? inv_i-1 : i+1].index;
            save_traversal_links(signs, child, next_index, links);
        }
    }
}

bvh build_generic_bvh(
    uint leaf_count,
    leaf_node* leaves,
    bvh_buffers& bc
){
    build_node root;
    root.min = float3{FLT_MAX,FLT_MAX,FLT_MAX};
    root.max = float3{-FLT_MAX,-FLT_MAX,-FLT_MAX};
    for(uint i = 0; i < leaf_count; ++i)
    {
        root.min = fmin(root.min, leaves[i].min);
        root.max = fmax(root.max, leaves[i].max);
    }

    build_recursive_sah(leaves, leaf_count, root);

    bvh b;
    b.node_offset = bc.nodes.size();
    uint node_index = 0;
    traverse_bvh_nodes_bfs(root, node_index, bc.nodes);
    b.node_count = bc.nodes.size() - b.node_offset;

    // Traverse the BVH in various ways to produce the link lists.
    bc.links.resize(bc.links.size() + 8 * b.node_count);
    for(int i = 0; i < 8; ++i)
    {
        bool signs[3] = {bool(i&1), bool(i&2), bool(i&4)};
        save_traversal_links(
            signs, root, 0xFFFFFFFFu,
            bc.links.data() + 8 * b.node_offset + i * b.node_count
        );
    }

    return b;
}

bvh build_blas(mesh m, const mesh_buffers& buffers, bvh_buffers& bc)
{
    std::vector<leaf_node> leaves;
    for(uint i = 0; i < m.triangle_count; ++i)
    {
        uint i0 = buffers.indices[m.index_offset + i * 3 + 0];
        uint i1 = buffers.indices[m.index_offset + i * 3 + 1];
        uint i2 = buffers.indices[m.index_offset + i * 3 + 2];
        float3 pos0 = buffers.pos[m.base_vertex_offset + i0];
        float3 pos1 = buffers.pos[m.base_vertex_offset + i1];
        float3 pos2 = buffers.pos[m.base_vertex_offset + i2];

        leaves.push_back({
            fmin(pos0, fmin(pos1, pos2)),
            fmax(pos0, fmax(pos1, pos2)),
            i
        });
    }
    return build_generic_bvh(leaves.size(), leaves.data(), bc);
}

bvh build_tlas(
    uint instance_count,
    const std::pair<const tlas_instance*, uint>* ti,
    const bvh_buffers& bc_in,
    bvh_buffers& bc_out
){
    std::vector<leaf_node> leaves;
    for(uint i = 0; i < instance_count; ++i)
    {
        const tlas_instance& inst = *ti[i].first;
        bvh_node node = bc_in.nodes[inst.blas.node_offset];

        leaf_node leaf;
        leaf.index = ti[i].second;
        float3 bounds[2] = {
            {node.min_x, node.min_y, node.min_z},
            {node.max_x, node.max_y, node.max_z},
        };
        for(int a = 0; a < 8; ++a)
        {
            float4 v = mul_m4v4(
                inst.transform,
                float4{bounds[a&1].x, bounds[a&2?0:1].y, bounds[a&4?0:1].z, 1}
            );
            float3 c = float3{v.x, v.y, v.z};

            leaf.min = a == 0 ? c : fmin(c, leaf.min);
            leaf.max = a == 0 ? c : fmax(c, leaf.max);
        }
        leaves.push_back(leaf);
    }
    return build_generic_bvh(leaves.size(), leaves.data(), bc_out);
}

void pop_bvh(bvh_buffers& bc, bvh& as)
{
    if(as.node_count == 0) return;
    bc.nodes.resize(as.node_offset);
    bc.links.resize(as.node_offset*8u);
    as.node_count = 0;
}
