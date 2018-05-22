# InstancedRendering
This originally started out as an attempt to try out instanced rendering. It now includes an OpenCL wind simulation.

The program is written in C++/Qt. OpenGL and OpenCL are both used, but some of the OpenCL code is Apple-specific (guarded by `#ifdef __APPLE__`). Fortunately, these instances are completely isolated to two files: `include_opencl.h` and `myclwrapper.cpp`.
1) The `cl_interface/include_opencl.h` header does a platform-specific OpenCL include. For Apple, this is `<OpenCL/opencl.h>`, and for other systems it might be different. By default, if `__APPLE__` isn't defined, I include `<cl.h>`.
2) Change `makeCLGLContext()` in `MyCLWrapper`.

## Controls
Press F to toggle the force. The quad in the upper-right corner displays the wind velocities. The grass reacts to the wind.

## Features
- 128x128 grass blades are drawn with a `glDrawArraysInstanced()` call.
- OpenCL code approximates the Navier-Stokes equations for an incompressible fluid.
- The grass waves in response to the wind.

## Specifics
A small script creates the grass blade model. Per-instance data is stored in special per-instance buffers. I use `glVertexAttribDivisor()` and `glDrawArraysInstanced()` to achieve this.

To simulate wind, I use the Navier-Stokes approximation described [here](http://developer.download.nvidia.com/books/HTML/gpugems/gpugems_ch38.html). My code for this is currently quite messy.
