#include "scene.hh"
#include "path_tracer.hh"
#include "bmp.hh"
#include <clocale>
#include <memory>

#include "helper.hh"


// Renders the given scene into an image using path tracing.
void baseline_render(const scene& s, uchar4* image)
{
    float3 colors[IMAGE_WIDTH * IMAGE_HEIGHT];
        
    for(uint i = 0; i < IMAGE_WIDTH * IMAGE_HEIGHT; ++i)
    {
        uint x = i % IMAGE_WIDTH;
        uint y = i / IMAGE_WIDTH; 

        colors[i] = {0, 0, 0};

        for(uint j = 0; j < SAMPLES_PER_PIXEL; ++j)
        {
            colors[i] += path_trace_pixel(
                uint2{x, y},
                j,
                s.subframes.data(),
                s.instances.data(),
                s.bvh_buf.nodes.data(),
                s.bvh_buf.links.data(),
                s.mesh_buf.indices.data(),
                s.mesh_buf.pos.data(),
                s.mesh_buf.normal.data(),
                s.mesh_buf.albedo.data(),
                s.mesh_buf.material.data()
            );
        }

        
        colors[i] /= SAMPLES_PER_PIXEL;
        image[i] = tonemap_pixel(colors[i]);
        
    }
}





int main()
{
    // Make sure all text parsing is unaffected by locale
    setlocale(LC_ALL, "C");
    
    auto start_0 = std::chrono::high_resolution_clock::now();
    auto start = start_0;
    scene s = load_scene();
    auto now = std::chrono::high_resolution_clock::now();
    std::cout << "EXECUTION TIME OF load_scene() : " << (now - start) << "ms" << "\n";


    std::unique_ptr<uchar4[]> image(new uchar4[IMAGE_WIDTH * IMAGE_HEIGHT]);

    uint frame_count = 1;//get_animation_frame_count(s);

    std::cout << frame_count << "\n";

    for(uint frame_index = 0; frame_index < frame_count; ++frame_index)
    {
        // Update scene state for the current frame & render it
        start = std::chrono::high_resolution_clock::now();
        setup_animation_frame(s, frame_index);
        now = std::chrono::high_resolution_clock::now();
        std::cout << "FRAME #" << frame_index << " EXECUTION TIME OF setup_animation_frame() : " << (now - start) << "ms" << "\n";
        
        
        start = std::chrono::high_resolution_clock::now();
        baseline_render(s, image.get());
        now = std::chrono::high_resolution_clock::now();
        std::cout << "FRAME #" << frame_index << " EXECUTION TIME OF baseline_render() : " << (now - start) << "ms" << "\n";

        // Create string for the index number of the frame with leading zeroes.
        std::string index_str = std::to_string(frame_index);
        while(index_str.size() < 4) index_str.insert(index_str.begin(), '0');

        // Write output image
        write_bmp(
            ("output/frame_"+index_str+".bmp").c_str(),
            IMAGE_WIDTH, IMAGE_HEIGHT, 4, IMAGE_WIDTH * 4,
            (uint8_t*)image.get()
        );
    }
    now = std::chrono::high_resolution_clock::now();
    
    std::cout << "\n\nEXECUTION TIME OF PROGRAM FOR " << frame_count << " FRAMES: " << (now - start_0) << "ms" << "\n";

    return 0;
}
