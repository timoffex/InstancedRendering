#ifndef GRASSWINDCLPROGRAM_H
#define GRASSWINDCLPROGRAM_H

#include "myclwrapper.h"

#include <OpenCL/opencl.h>

class GrassWindCLProgram
{
public:
    GrassWindCLProgram();

    bool create(MyCLWrapper *wrapper);

    /// Runs the kernel with the given parameters. Does NOT
    /// call clFinish().
    ///
    /// Note that grassWindPositions and grassWindVelocities
    /// are modified by this operation. Returns true on
    /// success, false on failure.
    bool reactToWind(cl_mem grassWindPositions,
                     cl_mem grassWindVelocities,
                     cl_mem grassWindStrength,
                     cl_uint numBlades,
                     cl_float dt);

private:
    bool mCreated;

    MyCLWrapper *mCLWrapper;

    cl_program mProgram;
    cl_kernel mGrassReactKernel;
};

#endif // GRASSWINDCLPROGRAM_H
