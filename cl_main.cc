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
static cl_platform_id *platforms;
static cl_device_id *devices;
static cl_uint numDevices = 0;
static cl_program program;
static cl_kernel main_kernel; 

static cl_mem subframes_buf;
static cl_mem instances_buf;
static cl_mem bvh_nodes_buf;
static cl_mem bvh_links_buf;
static cl_mem mesh_indices_buf;
static cl_mem mesh_pos_buf;
static cl_mem mesh_normal_buf;
static cl_mem mesh_albedo_buf;
static cl_mem mesh_material_buf;
static cl_mem image_buf;


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

}


void
destroy() {
    // Free OpenCL resources
    clReleaseKernel(main_kernel);
    clReleaseCommandQueue(cmdQueue);
    clReleaseMemObject(subframes_buf);
    clReleaseMemObject(instances_buf);
    clReleaseMemObject(bvh_nodes_buf);
    clReleaseMemObject(bvh_links_buf);
    clReleaseMemObject(mesh_indices_buf);
    clReleaseMemObject(mesh_pos_buf);
    clReleaseMemObject(mesh_normal_buf);
    clReleaseMemObject(mesh_albedo_buf);
    clReleaseMemObject(mesh_material_buf);
    clReleaseMemObject(image_buf);

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

    int frame_index = 0;

    setup_animation_frame(s, frame_index);



    
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


    subframes_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.subframes.size(),
          NULL, &status);
    chk(status, "clCreateBuffer");

    instances_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.instances.size(),
          NULL, &status);
    chk(status, "clCreateBuffer");

    bvh_nodes_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.bvh_buf.nodes.size(),
          NULL, &status);
    chk(status, "clCreateBuffer");

    bvh_links_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.bvh_buf.links.size(),
          NULL, &status);
    chk(status, "clCreateBuffer");

    mesh_indices_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.mesh_buf.indices.size(),
          NULL, &status);
    chk(status, "clCreateBuffer");

    mesh_pos_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.mesh_buf.pos.size(),
          NULL, &status);
    chk(status, "clCreateBuffer");

    mesh_normal_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.mesh_buf.normal.size(),
          NULL, &status);
    chk(status, "clCreateBuffer");

    mesh_albedo_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.mesh_buf.albedo.size(),
          NULL, &status);
    chk(status, "clCreateBuffer");

    mesh_material_buf = clCreateBuffer(context, CL_MEM_READ_ONLY, s.mesh_buf.material.size(),
          NULL, &status);
    chk(status, "clCreateBuffer");

    image_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, IMAGE_HEIGHT*IMAGE_WIDTH*sizeof(uchar4),
          NULL, &status);
    chk(status, "clCreateBuffer");

    printf("Opencl Memory Objects created successfully \n\n");


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


    // Write input array A to the device buffer bufferA
    cl_event write_event_00, write_event_1, write_event_2, write_event_3, 
        write_event_4, write_event_5, write_event_6, write_event_7, write_event_8, write_event_9, write_event_10;
    
    status = clEnqueueWriteBuffer(cmdQueue, subframes_buf, CL_TRUE,
       0, s.subframes.size(), s.subframes.data(), 0, NULL, &write_event_1);
    
    status |= clEnqueueWriteBuffer(cmdQueue, instances_buf, CL_TRUE,
       0, s.instances.size(), s.instances.data(), 0, NULL, &write_event_2);
    
    status |= clEnqueueWriteBuffer(cmdQueue, bvh_nodes_buf, CL_TRUE,
       0, s.bvh_buf.nodes.size(), s.bvh_buf.nodes.data(), 0, NULL, &write_event_3);
    
    status |= clEnqueueWriteBuffer(cmdQueue, bvh_links_buf, CL_TRUE,
       0, s.bvh_buf.links.size(), s.bvh_buf.links.data(), 0, NULL, &write_event_4);
    
    status |= clEnqueueWriteBuffer(cmdQueue, mesh_indices_buf, CL_TRUE,
       0, s.mesh_buf.indices.size(), s.mesh_buf.indices.data(), 0, NULL, &write_event_5);

    status |= clEnqueueWriteBuffer(cmdQueue, mesh_pos_buf, CL_TRUE,
       0, s.mesh_buf.pos.size(), s.mesh_buf.pos.data(), 0, NULL, &write_event_6);
    
    status |= clEnqueueWriteBuffer(cmdQueue, mesh_normal_buf, CL_TRUE,
       0, s.mesh_buf.normal.size(), s.mesh_buf.normal.data(), 0, NULL, &write_event_7);

    status |= clEnqueueWriteBuffer(cmdQueue, mesh_albedo_buf, CL_TRUE,
       0, s.mesh_buf.albedo.size(), s.mesh_buf.albedo.data(), 0, NULL, &write_event_8);

    status |= clEnqueueWriteBuffer(cmdQueue, mesh_material_buf, CL_TRUE,
       0, s.mesh_buf.material.size(), s.mesh_buf.material.data(), 0, NULL, &write_event_9);

    chk(status, "clEnqueueWriteBuffer");
    
    printf("Write Events to memory buf successful \n\n"); 
    

    // Associate the input and output buffers with the kernel
    status  = clSetKernelArg(main_kernel, 0, sizeof(cl_mem), &subframes_buf);
    status |= clSetKernelArg(main_kernel, 1, sizeof(cl_mem), &instances_buf);
    status |= clSetKernelArg(main_kernel, 2, sizeof(cl_mem), &bvh_nodes_buf);
    status |= clSetKernelArg(main_kernel, 3, sizeof(cl_mem), &bvh_links_buf);
    status |= clSetKernelArg(main_kernel, 4, sizeof(cl_mem), &mesh_indices_buf);
    status |= clSetKernelArg(main_kernel, 5, sizeof(cl_mem), &mesh_pos_buf);
    status |= clSetKernelArg(main_kernel, 6, sizeof(cl_mem), &mesh_normal_buf);
    status |= clSetKernelArg(main_kernel, 7, sizeof(cl_mem), &mesh_albedo_buf);
    status |= clSetKernelArg(main_kernel, 8, sizeof(cl_mem), &mesh_material_buf);
    status |= clSetKernelArg(main_kernel, 9, sizeof(cl_mem), &image_buf);
    chk(status, "clSetKernelArg");

    printf("Kernel Arguments set successfully \n\n"); 
    

    // Define an index space (global work size) of work
    // items for execution. A workgroup size (local work size)
    // is not required, but can be used.
    size_t globalWorkSize_sobel[1];
    globalWorkSize_sobel[0] = IMAGE_WIDTH;
    globalWorkSize_sobel[1] = IMAGE_HEIGHT;



    // cl_event read_event1;

    // status = clEnqueueReadBuffer(cmdQueue, image_buf, CL_TRUE, 0,
    //     IMAGE_HEIGHT*IMAGE_HEIGHT*sizeof(uchar4), image, 0, NULL, &read_event1);
    // chk(status, "clEnqueueReadBuffer");
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