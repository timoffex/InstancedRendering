#ifndef GRASSWINDCLPROGRAM_H
#define GRASSWINDCLPROGRAM_H

#include "cl_interface/myclwrapper.h"

#include "cl_interface/include_opencl.h"

#include "cl_interface/myclprogram.h"
#include "cl_interface/myclkernel.h"

class GrassWindCLProgram
{
public:
    GrassWindCLProgram();
    ~GrassWindCLProgram();

    /// Creates the program and its kernels. This program will
    /// use the given MyCLWrapper object but will not own it.
    bool create(MyCLWrapper *wrapper);

    /// Lets go of all resources (except the MyCLWrapper object).
    void release();


    /// Simulates grass response to wind. Does NOT call clFinish().
    ///
    /// Note that grassWindPositions and grassWindVelocities are modified by
    /// this operation. Returns true on success, false on failure.
    bool reactToWind2(cl_mem grassWindOffsets,
                      cl_mem grassPeriodOffsets,
                      cl_mem grassNormalizedPositions,
                      cl_image windVelocity,
                      cl_uint numBlades,
                      cl_float time);

private:

    using GrassReactKernelType = MyCLKernel<cl_mem,
                                            cl_mem,
                                            cl_mem,
                                            cl_image,
                                            cl_uint,
                                            cl_float>;


    bool mCreated;

    MyCLWrapper *mCLWrapper;

    MyCLProgram mProgram;
    GrassReactKernelType mGrassReact2Kernel;
};

#endif // GRASSWINDCLPROGRAM_H
