OPENCL_INCLUDE_PATH = /opt/AMDAPP/include
OPENCL_LIB_PATH = /opt/AMDAPP/lib/x86_64


CC=g++
CFLAGS=-std=c++17 -O3 -ffast-math -march=native
LDFLAGS=-I${OPENCL_INCLUDE_PATH} -L${OPENCL_LIB_PATH} -lOpenCL  -lm
SRCS_CL=cl_main.cc
SRCS_DEFAULT=main.cc
SRCS=bvh.cc mesh.cc bmp.cc scene.cc opencl_util.c

OBJS=$(SRCS:.cc=.o)
OBJS_CL=$(OBJS) $(SRCS_CL:.cc=.o)
OBJS_DEFAULT=$(OBJS) $(SRCS_DEFAULT:.cc=.o)


HEADERS=bvh.hh mesh.hh math.hh scene.hh ray_query.hh config.hh bmp.hh path_tracer.hh helper.hh opencl_util.h




.PHONY: all clean
all: $(SRCS) pt pt_main

pt: $(OBJS_CL)
	@$(CC)  $(OBJS_CL) -o $@ $(LDFLAGS) 

pt_main: $(OBJS_DEFAULT)
	@$(CC)  $(OBJS_DEFAULT) -o $@ $(LDFLAGS) 

%.o: %.cc $(HEADERS)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o
	rm pt
	rm pt_main
