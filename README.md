HPC project skeleton
====================

This program renders a short animation using a photorealistic rendering
algorithm called path tracing, which is often used in 3D animation movie
production.

In its initial state, the program uses only one CPU core for rendering the
animation. Your task is to modify the program such that it renders all of the
animation frames as fast as possible, without harming the resulting image
quality.

## Building

First off, read the comments in `config.hh` and fill in your STUDENT_ID.
The program has no dependencies. You can build it using `make`, with the
included Makefile. You are allowed to add dependencies if needed.

## File tour

You will need to modify `main.cc` and `Makefile` substantially, so please read
them carefully and make sure you understand what they are doing.

Most of the files are written in C++17. Although, certain header files are
written such that they can also be included into a C99 program if needed. These
headers are:

| Name             | Usage                           |
|------------------|---------------------------------|
| `config.hh`      | Rendering parameters            |
| `bvh.hh`         | Data structures for ray tracing |
| `math.hh`        | Various mathematical utilities  |
| `mesh.hh`        | 3D models                       |
| `path_tracer.hh` | Path tracing algorithm          |
| `ray_query.hh`   | Ray traversal algorithm         |
| `scene.hh`       | 3D scene                        |

In practice, you can include `path_tracer.hh` in a C99 program and use the
functions `path_trace_pixel` and `tonemap_pixel`.

As you implement various parallel computing techniques, you may have to do
small, superficial changes to a few of these headers, but essentially only when
you get compiler warnings related to them. You do not have to understand the
algorithms implemented in the headers.

You shouldn't need to modify the rest of the files, although you may want to
look at some function declarations in `bmp.hh` and `scene.hh` just so you know
what `main.cc` is doing. You are allowed to add new files freely.

## Basic algorithm

See `baseline_render` in `main.cc`. When rendering a pixel, we need to
calculate the average value of path_trace_pixel with thousands of different
random seeds. The seed and motion blur behaviour is determined by the
`sample_index` parameter given to `path_trace_pixel`. When you write an
improved renderer, make sure that you don't change the behaviour of this
`sample_index` parameter, or you can easily break the image.

After the average color has been computed, it must be tonemapped before it is
saved to the image. `tonemap_pixel` turns our color from \[0, infinity\] (like
real life) to \[0, 255\] (like computer monitors) and applies sRGB correction.

## Validator

You can validate your final animation using the included `validator.py` script.
It will check that your frames are not corrupted. You'll need to download the 
reference frames from the course webpage. Assuming the reference frames are in a
folder called `reference` and your frames are in `output`, you can run the
validator as follows:

```sh
python3 validator.py reference output
```

The validator saves a file called `validation_result.txt`. Submit this to the
course staff along with your code. You can search the file for `BAD`. The word
is placed on the corrupted frames if there are any. If there are, FIX YOUR CODE!

## Have fun!

Once you have the frames, you can render them into a movie with ffmpeg:

```sh
ffmpeg -framerate 30 -pattern_type glob -i 'output/frame_*.bmp' -c:v libx264 -pix_fmt yuv420p movie.mp4
```
