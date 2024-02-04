OPENCL_INCLUDE_PATH = /opt/AMDAPP/include
OPENCL_LIB_PATH = /opt/AMDAPP/lib/x86_64

CC=g++
CFLAGS=-std=c++17 -O3 -ffast-math -march=native
LDFLAGS=-I${OPENCL_INCLUDE_PATH} -L${OPENCL_LIB_PATH} -lOpenCL -fopenmp  -lm
SRCS=main.cc bvh.cc mesh.cc bmp.cc scene.cc opencl_util.c
OBJS=$(SRCS:.cc=.o)
HEADERS=bvh.hh mesh.hh math.hh scene.hh ray_query.hh config.hh bmp.hh path_tracer.hh opencl_util.h



.PHONY: all clean
all: $(SRCS) pt

pt: $(OBJS)
	$(CC)  $(OBJS) -o $@ $(LDFLAGS) 

%.o: %.cc $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o
	rm pt
