#include "fluid2dsimulationclprogram.h"

#include "cl_interface/myclerrors.h"


Fluid2DSimulationCLProgram::Fluid2DSimulationCLProgram()
    : mCreated(false)
{
}

bool Fluid2DSimulationCLProgram::create(MyCLWrapper *wrapper)
{
    mCLWrapper = wrapper;


    if (!mProgram.create(wrapper, ":/compute/fluidSimulation.cl"))
    {
        qDebug() << "Failed to create fluid simulation program.";
        return false;
    }

#ifndef MAKE_KERNEL
#define MAKE_KERNEL(var, name)\
    if (!var.createFromProgram(wrapper, mProgram.program(), name))\
    {\
        qDebug() << "Failed to create " name " kernel.";\
        return false;\
    }

    MAKE_KERNEL(mJacobiKernel, "jacobi");
    MAKE_KERNEL(mAdvectKernel, "advect");
    MAKE_KERNEL(mDivergenceKernel, "divergence");
    MAKE_KERNEL(mGradientKernel, "gradient");
    MAKE_KERNEL(mAddScaledKernel, "addScaled");
    MAKE_KERNEL(mVelocityBoundaryKernel, "velocityBoundary");
    MAKE_KERNEL(mPressureBoundaryKernel, "pressureBoundary");
#undef MAKE_KERNEL
#else
    static_assert(false);
#endif

    mCreated = true;
    return true;
}

void Fluid2DSimulationCLProgram::release()
{
    mJacobiKernel.destroy();
    mAdvectKernel.destroy();
    mDivergenceKernel.destroy();
    mGradientKernel.destroy();
    mAddScaledKernel.destroy();
    mVelocityBoundaryKernel.destroy();
    mPressureBoundaryKernel.destroy();

    mProgram.destroy();

    mCreated = false;
}

bool Fluid2DSimulationCLProgram::update(MyCLImage2D &velocities,
                                        MyCLImage2D *forces,
                                        MyCLImage2D &pressure,
                                        MyCLImage2D &temp1,
                                        MyCLImage2D &temp2,
                                        cl_float gridSize,
                                        cl_float dt,
                                        cl_float density,
                                        cl_float viscosity)
{
    Q_ASSERT( mCreated );

    // These help keep track of where the most updated
    // data is stored. At the end, the updated data
    // must be stored in the original variables.
    MyCLImage2D *velocityImage = &velocities;
    MyCLImage2D *pressureImage = &pressure;

    MyCLImage2D *freeImage1 = &temp1;
    MyCLImage2D *freeImage2 = &temp2;

    MyCLImage2D *divergenceImage = nullptr;
    MyCLImage2D *gradientImage = nullptr;


    /* Algorithm:
        1) advect
        2) diffuse (optional)
        3) add forces (optional)
        4) update pressure
            i)   compute velocity field divergence
            ii)  solve Poisson equation (probably using Jacobi)
        5) subtract gradient of pressure from velocities
        6) enforce boundary conditions
     * */


    /* Step 1: Advection */
    if (!advect(*velocityImage, *velocityImage, *freeImage1, dt, gridSize))
    {
        qDebug() << "Failure in advection step.";
        return false;
    }
    std::swap(velocityImage, freeImage1);


    /* Step 2: Diffusion (optional) */
    if (viscosity > 0)
    {
        cl_float hh_vdt = gridSize * gridSize / (viscosity * dt);
        for (int iteration = 0; iteration < 30; ++iteration)
        {
            MyCLImage2D *t1 = velocityImage;
            MyCLImage2D *t2 = freeImage1;

            for (int subIteration = 0; subIteration < 2; ++subIteration)
            {
                if (!jacobi(*t1, *t1, *t2, hh_vdt, 4 + hh_vdt))
                {
                    qDebug() << "Failure in diffusion step.";
                    return false;
                }

                // Swap pointers.
                std::swap(t1, t2);
            }
        }
    }

    /* Step 3: Add forces (optional) */
    if (forces != nullptr)
    {
        if (!addScaled(*velocityImage, *forces, dt, *freeImage1))
        {
            qDebug() << "Failure in add-forces step.";
            return false;
        }

        std::swap(velocityImage, freeImage1);
    }

    /* Step 4: Update pressure */
    if (!divergence(*velocityImage, *freeImage1, gridSize))
    {
        qDebug() << "Failure in computing divergence.";
        return false;
    }

    Q_ASSERT( divergenceImage == nullptr );
    std::swap(freeImage1, divergenceImage);
    std::swap(freeImage1, freeImage2);
    // freeImage1 should be free once again.
    // freeImage2 is now nullptr.

    // NOTE: Doing too many of these breaks everything.
    for (int iteration = 0; iteration < 4; ++iteration)
    {
        MyCLImage2D *t1 = pressureImage;
        MyCLImage2D *t2 = freeImage1;

        for (int subIteration = 0; subIteration < 2; ++subIteration)
        {
            if (!jacobi(*t1, *divergenceImage, *t2, -gridSize * gridSize, 4))
            {
                qDebug() << "Failure in pressure computation.";
                return false;
            }

            std::swap(t1, t2);
        }
    }

    // We don't need divergence now.
    Q_ASSERT( freeImage2 == nullptr );
    std::swap(freeImage2, divergenceImage);

    /* Step 5: Subtract pressure gradient */
    if (!gradient(*pressureImage, *freeImage1, gridSize))
    {
        qDebug() << "Failure in pressure gradient computation.";
        return false;
    }

    Q_ASSERT( gradientImage == nullptr );
    std::swap(freeImage1, gradientImage);
    std::swap(freeImage1, freeImage2);
    // freeImage1 should be free once again.
    // freeImage2 is now nullptr.

    if (!addScaled(*velocityImage, *gradientImage, -1.0/density, *freeImage1))
    {
        qDebug() << "Failure in subtracting pressure gradient.";
        return false;
    }
    std::swap(freeImage1, velocityImage);



    /* Step 6: Enforce boundary conditions */
    if (!velocityBoundary(*velocityImage, *freeImage1))
    {
        qDebug() << "Failure enforcing velocity boundary.";
        return false;
    }
    std::swap(velocityImage, freeImage1);

    if (!pressureBoundary(*pressureImage, *freeImage1))
    {
        qDebug() << "Failure enforcing pressure boundary.";
        return false;
    }
    std::swap(pressureImage, freeImage1);


    if (velocityImage != &velocities)
    {
        if (!copy(*velocityImage, velocities))
        {
            qDebug() << "Failure in copy()";
            return false;
        }
    }

    if (pressureImage != &pressure)
    {
        if (!copy(*pressureImage, pressure))
        {
            qDebug() << "Failure in copy()";
            return false;
        }
    }

    return true;
}

bool Fluid2DSimulationCLProgram::copy(MyCLImage2D &from, MyCLImage2D &to)
{
    return addScaled(from, from, 0, to);
}

bool Fluid2DSimulationCLProgram::jacobi(MyCLImage2D &input,
                                        MyCLImage2D &b,
                                        MyCLImage2D &output,
                                        cl_float alpha,
                                        cl_float beta)
{
    return mJacobiKernel(output.width(), output.height(), input, b, output, alpha, 1.0 / beta);
}

bool Fluid2DSimulationCLProgram::advect(MyCLImage2D &quantity,
                                        MyCLImage2D &velocity,
                                        MyCLImage2D &output,
                                        cl_float dt,
                                        cl_float gridSize)
{

    return mAdvectKernel(output.width(), output.height(), quantity, velocity, output, dt / gridSize);
}

bool Fluid2DSimulationCLProgram::divergence(MyCLImage2D &vecField,
                                            MyCLImage2D &output,
                                            cl_float gridSize)
{
    return mDivergenceKernel(output.width(), output.height(), vecField, output, 1.0 / gridSize);
}


bool Fluid2DSimulationCLProgram::gradient(MyCLImage2D &func,
                                          MyCLImage2D &output,
                                          cl_float gridSize)
{
    return mGradientKernel(output.width(), output.height(), func, output, 1.0 / gridSize);
}

bool Fluid2DSimulationCLProgram::addScaled(MyCLImage2D &t1, MyCLImage2D &t2,
                                           cl_float multiplier,
                                           MyCLImage2D &sum)
{
    return mAddScaledKernel(sum.width(), sum.height(), t1, t2, multiplier, sum);
}


bool Fluid2DSimulationCLProgram::velocityBoundary(MyCLImage2D &img, MyCLImage2D &out)
{
    return mVelocityBoundaryKernel(out.width(), out.height(), img, out);
}

bool Fluid2DSimulationCLProgram::pressureBoundary(MyCLImage2D &img, MyCLImage2D &out)
{
    return mPressureBoundaryKernel(out.width(), out.height(), img, out);
}

