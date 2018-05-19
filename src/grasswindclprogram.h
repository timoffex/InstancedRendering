#ifndef GRASSWINDCLPROGRAM_H
#define GRASSWINDCLPROGRAM_H

#include "cl_interface/myclwrapper.h"

#include "cl_interface/include_opencl.h"

class GrassWindCLProgram
{
public:
    GrassWindCLProgram();

    bool create(MyCLWrapper *wrapper);

    /// Runs the reactToWind kernel with the given parameters. Does NOT call clFinish().
    ///
    /// Note that grassWindPositions and grassWindVelocities are modified by
    /// this operation. Returns true on success, false on failure.
    bool reactToWind(cl_mem grassWindPositions,
                     cl_mem grassWindVelocities,
                     cl_mem grassNormalizedOffsets,
                     cl_image windStrengthImage,
                     cl_uint numBlades,
                     cl_float dt);

    bool reactToWind2(cl_mem grassWindOffsets,
                      cl_mem grassPeriodOffsets,
                      cl_mem grassNormalizedPositions,
                      cl_image windVelocity,
                      cl_uint numBlades,
                      cl_float time);


    /// Runs the updateWind kernel. Does NOT call clFinish().
    bool updateWind(cl_image windSpeeds,
                    cl_image forces,
                    cl_float gridSize,
                    cl_float dt,
                    cl_float viscosity,
                    cl_image output,
                    unsigned int imageWidth,
                    unsigned int imageHeight);

    /// Updates the wind speeds by approximating Navier Stokes
    /// for incompressible fluids.
    bool updateWindNew(cl_image windSpeeds,
                       cl_image forces,
                       cl_float gridSize,
                       cl_float dt,
                       cl_float density,
                       cl_float viscosity,
                       cl_image pressure,
                       cl_image temp1,
                       cl_image temp2);

    /// Runs the zeroTexture kernel. Does NOT call clFinish().
    bool zeroTexture(cl_image img);


    bool jacobi(cl_image input,
                cl_image b,
                cl_image output,
                cl_float alpha,
                cl_float beta);

    bool advect(cl_image quantity,
                cl_image velocity,
                cl_image output,
                cl_float dt,
                cl_float gridSize);

    bool divergence(cl_image vecField,
                    cl_image output,
                    cl_float gridSize);

    bool gradient(cl_image func,
                  cl_image output,
                  cl_float gridSize);

    bool addScaled(cl_image t1, cl_image t2,
                   cl_float multiplier,
                   cl_image sum);

    bool velocityBoundary(cl_image img, cl_image out);

    bool pressureBoundary(cl_image img, cl_image out);

public:

    bool mCreated;

    MyCLWrapper *mCLWrapper;

    cl_program mProgram;
    cl_kernel mGrassReactKernel;
    cl_kernel mGrassReact2Kernel;
    cl_kernel mUpdateWindKernel;
    cl_kernel mZeroTextureKernel;

    cl_kernel mJacobiKernel;
    cl_kernel mAdvectKernel;
    cl_kernel mDivergenceKernel;
    cl_kernel mGradientKernel;
    cl_kernel mAddScaledKernel;
    cl_kernel mVelocityBoundaryKernel;
    cl_kernel mPressureBoundaryKernel;
};

#endif // GRASSWINDCLPROGRAM_H
