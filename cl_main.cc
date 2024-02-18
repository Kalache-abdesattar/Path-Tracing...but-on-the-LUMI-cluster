#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scene.hh"
#include "path_tracer.hh"
#include "bmp.hh"
#include <clocale>
#include <memory>

#include "helper.hh"


// OpenCL includes
#include <CL/cl.h>

#include "opencl_util.h"


static cl_context context;
static cl_command_queue cmdQueue;
static cl_mem buf_imagein, buf_sobelx, buf_sobely, buf_phase, buf_magnitude, buf_output;
static cl_platform_id *platforms;
static cl_device_id *devices;
static cl_uint numDevices = 0;
static cl_program program;
static cl_kernel main_kernel; 



void init() {

       cl_int status;

       // Retrieve the number of platforms
       cl_uint numPlatforms = 0;
       status = clGetPlatformIDs(0, NULL, &numPlatforms);
       chk(status, "clGetPlatformIDs");

       printf("THE NUMBER OF PLATFORMS IS : %d\n", numPlatforms);

       // Allocate enough space for each platform
       platforms = (cl_platform_id*)malloc(
           numPlatforms*sizeof(cl_platform_id));

       // Fill in the platforms
       status = clGetPlatformIDs(numPlatforms, platforms, NULL);
       chk(status, "clGetPlatformIDs");

       // Retrieve the number of devices
       status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0,
           NULL, &numDevices);
       chk(status, "clGetDeviceIDs");


       printf("THE NUMBER OF DEVICES IS : %d\n", numPlatforms);



       // Allocate enough space for each device
       devices = (cl_device_id*)malloc(
           numDevices*sizeof(cl_device_id));
        
       // Fill in the devices
       status = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL,
           numDevices, devices, NULL);
       chk(status, "clGetDeviceIDs");


       // Create a context and associate it with the devices
       context = clCreateContext(NULL, numDevices, devices, NULL,
           NULL, &status);
       chk(status, "clCreateContext");

       // Create a command queue and associate it with the device
       cmdQueue = clCreateCommandQueue(context, devices[0], CL_QUEUE_PROFILING_ENABLE,
           &status);
       chk(status, "clCreateCommandQueue");


    //    // Input image buffer
    //    // HOST -----> Sobel3x3 Kernel
    //    buf_imagein = clCreateBuffer(context, CL_MEM_READ_ONLY, image_size*sizeof(u_char),
    //       NULL, &status);
    //    chk(status, "clCreateBuffer");


    //    // ******Sobel3x3 Kernel Buffers********* \\
    //    //
    //    // Sobel3x3 -----> phaseAndMagnitude
    //    buf_sobelx = clCreateBuffer(context, CL_MEM_READ_WRITE, image_size*sizeof(short),
    //        NULL, &status);
    //    chk(status, "clCreateBuffer");

    //    buf_sobely = clCreateBuffer(context, CL_MEM_READ_WRITE, image_size*sizeof(short),
    //        NULL, &status);
    //    chk(status, "clCreateBuffer");



    //    // ******phaseAndMagnitude Kernel Buffers********* \\
    //    // phaseAndMagnitude -----> nonMaxSuppression
    //    buf_phase = clCreateBuffer(context, CL_MEM_READ_WRITE, image_size*sizeof(u_char),
    //        NULL, &status);
    //    chk(status, "clCreateBuffer");


    //    buf_magnitude = clCreateBuffer(context, CL_MEM_READ_WRITE, image_size*sizeof(short),
    //        NULL, &status);
    //    chk(status, "clCreateBuffer");


    //    // ******nonMaxSuppression Kernel Buffers********* \\
    //    //
    //    // nonMaxSuppression -----> HOST
    //    buf_output = clCreateBuffer(context, CL_MEM_READ_WRITE, image_size*sizeof(u_char),
    //        NULL, &status);
    //    chk(status, "clCreateBuffer");


    //    const char* source = readSource("canny.cl");
    //    // Create a program with source code
    //    program = clCreateProgramWithSource(context, 1, &source, NULL, &status);
    //    chk(status, "clCreateProgramWithSource");


    //   // Build (compile) the program for the device
    //   status = clBuildProgram(program, numDevices, devices,
    //       NULL, NULL, NULL);
    //   chk(status, "clBuildProgram");

    //   print_programBuild_logs(program, devices[0]);

    //   // Create the three kernels
    //   kernel_sobel = clCreateKernel(program, "sobel3x3", &status);
    //   chk(status, "clCreateKernel");

    //   kernel_phaseAndMagnitude = clCreateKernel(program, "phaseAndMagnitude", &status);
    //   chk(status, "clCreateKernel");

    //   kernel_nonMaxSuppression = clCreateKernel(program, "nonMaxSuppression", &status);
    //   chk(status, "clCreateKernel");

}


void
destroy() {
    // Free OpenCL resources
    // clReleaseKernel(kernel_sobel);
    // clReleaseKernel(kernel_nonMaxSuppression);
    // clReleaseKernel(kernel_phaseAndMagnitude);
    clReleaseCommandQueue(cmdQueue);
    clReleaseMemObject(buf_imagein);
    clReleaseMemObject(buf_output);
    clReleaseMemObject(buf_magnitude);
    clReleaseMemObject(buf_phase);
    clReleaseMemObject(buf_sobelx);
    clReleaseMemObject(buf_sobely);
    clReleaseContext(context);
    clReleaseProgram(program);

    // Free host resources
    free(platforms);
    free(devices);
}



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




int main(){
    init();

    // Make sure all text parsing is unaffected by locale
    setlocale(LC_ALL, "C");
        // Make sure all text parsing is unaffected by locale

    scene s = load_scene();

    printf("maybe that is why it's failing : size of s.subframes = %d \n\n", s.instances.size());

    
    // subframe* subframe_buf = (subframe*)(malloc(s.subframes.size()));
    // tlas_instance* instances_buf = (tlas_instance*)(malloc(s.instances.size()));
    // bvh_node* bvh_nodes_buf = (bvh_node*)(malloc(s.bvh_buf.nodes.size()));
    // bvh_link* bvh_links_buf = (bvh_link*)(malloc(s.bvh_buf.links.size()));
    // uint* mesh_indices_buf = (uint*)(malloc(s.mesh_buf.indices.size()));
    // float3* mesh_pos_buf = (float3*)(malloc(s.mesh_buf.pos.size()));
    // float3* mesh_normal_buf = (float3*)(malloc(s.mesh_buf.normal.size()));
    // float4* mesh_albedo_buf = (float4*)(malloc(s.mesh_buf.albedo.size()));
    // float4* mesh_material_buf = (float4*)(malloc(s.mesh_buf.material.size()));

    int status = 0;



    // cl_mem subframe_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, (size_t)s.subframes.size(),
    //       NULL, &status);
    // chk(status, "clCreateBuffer");

    cl_mem instances_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.instances.size(),
          NULL, &status);
    chk(status, "clCreateBuffer");

    // cl_mem bvh_nodes_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.bvh_buf.nodes.size(),
    //       NULL, &status);
    // chk(status, "clCreateBuffer");

    // cl_mem bvh_links_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.bvh_buf.links.size(),
    //       NULL, &status);
    // chk(status, "clCreateBuffer");

    // cl_mem mesh_indices_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.mesh_buf.indices.size(),
    //       NULL, &status);
    // chk(status, "clCreateBuffer");

    // cl_mem mesh_pos_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.mesh_buf.pos.size(),
    //       NULL, &status);
    // chk(status, "clCreateBuffer");

    // cl_mem mesh_normal_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.mesh_buf.normal.size(),
    //       NULL, &status);
    // chk(status, "clCreateBuffer");

    // cl_mem mesh_albedo_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.mesh_buf.albedo.size(),
    //       NULL, &status);
    // chk(status, "clCreateBuffer");

    // cl_mem mesh_material_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.mesh_buf.material.size(),
    //       NULL, &status);
    // chk(status, "clCreateBuffer");

    // cl_mem image_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, IMAGE_HEIGHT*IMAGE_WIDTH*sizeof(uchar4),
    //       NULL, &status);
    // chk(status, "clCreateBuffer");


    const char* source = read_source("kernels.cl");
    // Create a program with source code
    program = clCreateProgramWithSource(context, 1, &source, NULL, &status);
    chk(status, "clCreateProgramWithSource"); 
    

    // Build (compile) the program for the device
    status = clBuildProgram(program, numDevices, devices,
        NULL, NULL, NULL);
    chk(status, "clBuildProgram");

    print_programBuild_logs(program, devices[0]);


    printf("Program created successfully \n\n");



    main_kernel = clCreateKernel(program, "baseline_render", &status);
    chk(status, "clCreateKernel");

    printf("Kernel Object created successfully \n\n");
    
    
    // std::unique_ptr<uchar4[]> image(new uchar4[IMAGE_WIDTH * IMAGE_HEIGHT]);

    // uint frame_count = 1;//get_animation_frame_count(s);

    // for(uint frame_index = 0; frame_index < frame_count; ++frame_index)
    // {
    //     // Update scene state for the current frame & render it
    //     setup_animation_frame(s, frame_index);
        
    //     baseline_render(s, image.get());
        
    //     // Create string for the index number of the frame with leading zeroes.
    //     std::string index_str = std::to_string(frame_index);
    //     while(index_str.size() < 4) index_str.insert(index_str.begin(), '0');

    //     // Write output image
    //     write_bmp(
    //         ("output/frame_"+index_str+".bmp").c_str(),
    //         IMAGE_WIDTH, IMAGE_HEIGHT, 4, IMAGE_WIDTH * 4,
    //         (uint8_t*)image.get()
    //     );
    // }

    return 0;
}