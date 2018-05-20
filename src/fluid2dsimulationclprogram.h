#ifndef FLUID2DSIMULATIONCLPROGRAM_H
#define FLUID2DSIMULATIONCLPROGRAM_H

#include "cl_interface/include_opencl.h"
#include "cl_interface/myclwrapper.h"
#include "cl_interface/myclimage.h"

class Fluid2DSimulationCLProgram
{
public:
    Fluid2DSimulationCLProgram();

    bool create(MyCLWrapper *wrapper);

    void release();

    /// Updates the velocities and pressure images.
    ///
    /// If the viscosity <= 0, the diffusion step is skipped.
    /// If forces == NULL, the force application step is skipped.
    bool update(MyCLImage2D &velocities,
                MyCLImage2D *forces,
                MyCLImage2D &pressure,
                MyCLImage2D &temp1,
                MyCLImage2D &temp2,
                cl_float gridSize,
                cl_float dt,
                cl_float density,
                cl_float viscosity);


    bool copy(MyCLImage2D &from, MyCLImage2D &to);

    bool jacobi(MyCLImage2D &input,
                MyCLImage2D &b,
                MyCLImage2D &output,
                cl_float alpha,
                cl_float beta);

    bool advect(MyCLImage2D &quantity,
                MyCLImage2D &velocity,
                MyCLImage2D &output,
                cl_float dt,
                cl_float gridSize);

    bool divergence(MyCLImage2D &vecField,
                    MyCLImage2D &output,
                    cl_float gridSize);

    bool gradient(MyCLImage2D &func,
                  MyCLImage2D &output,
                  cl_float gridSize);

    bool addScaled(MyCLImage2D &t1, MyCLImage2D &t2,
                   cl_float multiplier,
                   MyCLImage2D &sum);

    /* TODO: Instead of using OpenCL, I should draw lines on
            a given image by using OpenGL. */
    bool velocityBoundary(MyCLImage2D &img, MyCLImage2D &out);
    bool pressureBoundary(MyCLImage2D &img, MyCLImage2D &out);

private:
    bool mCreated;

    MyCLWrapper *mCLWrapper;

    cl_program mProgram;
    cl_kernel mJacobiKernel;
    cl_kernel mAdvectKernel;
    cl_kernel mDivergenceKernel;
    cl_kernel mGradientKernel;
    cl_kernel mAddScaledKernel;
    cl_kernel mVelocityBoundaryKernel;
    cl_kernel mPressureBoundaryKernel;
};

#endif // FLUID2DSIMULATIONCLPROGRAM_H
