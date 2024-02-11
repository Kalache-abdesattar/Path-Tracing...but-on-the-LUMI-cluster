CC = gcc
OPENCL_INCLUDE_PATH = /opt/AMDAPP/include
OPENCL_LIB_PATH = /opt/AMDAPP/lib/x86_64


.PHONY: all clean 
all: cl_main.c opencl_util.c opencl_util.h
	$(CC) -g -o cl_main cl_main.c opencl_util.c -I${OPENCL_INCLUDE_PATH} -L${OPENCL_LIB_PATH} -lOpenCL -lm

clean: 
	rm cl_main.o