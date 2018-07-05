#ifndef FLUID2DSIMULATION_H
#define FLUID2DSIMULATION_H

#include "fluid2dsimulationclprogram.h"

#include "cl_interface/myclwrapper.h"
#include "cl_interface/myclimage.h"
#include "cl_interface/include_opencl.h"

#include <QOpenGLTexture>
#include <QDebug>

struct Fluid2DSimulationConfig
{
    /// Creates an inviscid (viscosity = 0) fluid with default density and grid coarseness.
    Fluid2DSimulationConfig(size_t width, size_t height, float dens = 1, float gridSquare = 0.1f)
        : width(width),
          height(height),
          hasViscosity(false),
          viscosity(0),
          density(dens),
          gridSquareSize(gridSquare),
          zeroInitializeSharedTextures(true)
    {
    }

    /// Helper to enable and set the viscosity. The parameter should be > 0.
    void setViscosity(float visc)
    {
        if (visc <= 0)
        {
            qWarning() << "Trying to set nonpositive viscosity disables viscosity.";
            hasViscosity = false;
            visc = 0;
        }
        else
        {
            hasViscosity = true;
            viscosity = visc;
        }
    }

    /// The width of the grid in grid-squares.
    size_t width;

    /// The height of the grid in grid-squares.
    size_t height;

    /// Whether this fluid has viscosity.
    bool hasViscosity;
    float viscosity;

    float density;

    /// The side-length of a single square in the grid.
    float gridSquareSize;

    /// Whether to zero-initialize the given OpenGL textures.
    bool zeroInitializeSharedTextures;
};

class Fluid2DSimulation
{
public:
    Fluid2DSimulation(Fluid2DSimulationConfig config);
    ~Fluid2DSimulation();

    /// Creates the fluid simulation, optionally using the given OpenGL textures
    /// for some storage (so that the output may be visualized).
    bool create(MyCLWrapper *wrapper,
                const QOpenGLTexture *velocityTexture = nullptr,
                const QOpenGLTexture *pressureTexture = nullptr);


    /// Releases the OpenCL objects created in create(). Releases nothing other than that
    /// (e.g. doesn't release the OpenCL context or the OpenGL textures).
    void release();

    /// Updates the fluid without applying forces.
    bool update(float dtSeconds);

    /// Updates the fluid, applying forces.
    bool update(float dtSeconds, MyCLImage2D &forces);

    size_t gridWidth() const { return mConfig.width; }
    size_t gridHeight() const { return mConfig.height; }

    const MyCLImage2D &velocities() const { return mVelocities; }
    MyCLImage2D &velocities() { return mVelocities; }

    const MyCLImage2D &pressure() const { return mPressure; }
    MyCLImage2D &pressure() { return mPressure; }

private:
    bool createImages(MyCLWrapper *wrapper,
                      const QOpenGLTexture *velocityTexture = nullptr,
                      const QOpenGLTexture *pressureTexture = nullptr);

    bool mInitialized;

    MyCLWrapper *mCLWrapper;

    Fluid2DSimulationConfig mConfig;
    Fluid2DSimulationCLProgram mFluidProgram;

    MyCLImage2D mVelocities;
    MyCLImage2D mPressure;
    MyCLImage2D mTemp2F_1;
    MyCLImage2D mTemp2F_2;
};

#endif // FLUID2DSIMULATION_H
