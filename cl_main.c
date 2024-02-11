#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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


// Needed only in Part 2 for OpenCL initialization
void
init() {

       cl_int status;

       // Retrieve the number of platforms
       cl_uint numPlatforms = 0;
       status = clGetPlatformIDs(0, NULL, &numPlatforms);
       chk(status, "clGetPlatformIDs");

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


void main(){
    init();

}