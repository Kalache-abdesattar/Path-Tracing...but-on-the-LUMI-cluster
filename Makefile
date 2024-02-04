CC=g++
CFLAGS=-std=c++17 -g -O3 -ffast-math -march=native
#CFLAGS=-std=c++17 -g
LDFLAGS=-lm
SRCS=main.cc bvh.cc mesh.cc bmp.cc scene.cc
OBJS=$(SRCS:.cc=.o)
HEADERS=bvh.hh mesh.hh math.hh scene.hh ray_query.hh config.hh bmp.hh path_tracer.hh

.PHONY: all clean
all: $(SRCS) pt

pt: $(OBJS)
	$(CC)  $(OBJS) -o $@ $(LDFLAGS)

%.o: %.cc $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm *.o
	rm pt
