/* COMP.CE.350 Parallelization Exercise util functions
   Copyright (c) 2023 Topi Leppanen topi.leppanen@tuni.fi
*/

#ifndef OPENCL_UTIL_H
#define OPENCL_UTIL_H

#include <CL/cl.h>

struct profiling_info{
    cl_ulong start;
    cl_ulong end;
};


const char* clErrorString(int e);
char* read_source(char* kernelPath);
cl_ulong getStartEndTime(cl_event event);
float ocl_profile(cl_event* events, int num_events, char** event_type);
void chk(cl_int status, const char* cmd);
void print_programBuild_logs(cl_program program, cl_device_id device);

#endif