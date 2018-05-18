# InstancedRendering
This originally started out as an attempt to try out instanced rendering. It now includes an OpenCL wind simulation.

The program is written in C++/Qt. OpenGL and OpenCL are both used, but some of the OpenCL code is Apple-specific (guarded by `#ifdef __APPLE__`). Adapting this to other platforms shouldn't be hard:
1) Change all instances of `#include <OpenCL/opencl.h>` to the appropriate includes.
2) Change `makeCLGLContext()` in `MyCLWrapper`.

## Features
- 128x128 grass blades drawn with a `glDrawArraysInstanced()` call
- OpenCL code approximating the Navier-Stokes equations for an incompressible fluid
- grass waves in response to the wind

## Specifics
A small script creates the grass blade model. Per-instance data is stored in special per-instance buffers. I use `glVertexAttribDivisor()` to correctly pass this data to the shader.

To simulate wind, I use the Navier-Stokes approximation described [here](http://developer.download.nvidia.com/books/HTML/gpugems/gpugems_ch38.html). My code for this is currently quite messy.
