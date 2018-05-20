#include "fluid2dsimulationclprogram.h"

#include "cl_interface/myclerrors.h"

#include <QByteArray>
#include <QFile>
#include <QString>
#include <QIODevice>
#include <QDebug>

/// Gets the source code for the program.
static QByteArray getSource();

/// Finds the next multiple of `factor` greater than
/// or equal to `startVal`.
static size_t nextMultiple(size_t startVal, size_t factor);

/// Factors num into f1 * f2 such that f1 and f2 are
/// close to each other.
static void factorEvenly(size_t num, size_t *f1, size_t *f2);

static cl_int run2DKernel(cl_command_queue queue, cl_device_id device, cl_kernel kernel, size_t globalSize1, size_t globalSize2);


Fluid2DSimulationCLProgram::Fluid2DSimulationCLProgram()
    : mCreated(false)
{
}

bool Fluid2DSimulationCLProgram::create(MyCLWrapper *wrapper)
{
    /* Steps:
        1) Create a program using clCreateProgramWithSource()
        2) Build the program using clBuildProgram()
        3) Create the kernels with clCreateKernel() (a program may
            contain multiple kernels)
    */

    mCLWrapper = wrapper;

    QByteArray sourceCode = getSource();
    const char *data = sourceCode.data();

    cl_int err;

    mProgram = clCreateProgramWithSource(wrapper->context(), 1, &data, NULL, &err);

    if (!mProgram || err != CL_SUCCESS)
    {
        // TODO create error parse for this
        qDebug() << "Failed to create program. Program: " << mProgram;
        return false;
    }

    err = clBuildProgram(mProgram, 0, NULL, NULL, NULL, NULL);

    if (err != CL_SUCCESS)
    {
        size_t length;
        char buffer[2048];

        clGetProgramBuildInfo(mProgram, wrapper->device(), CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &length);

        qDebug() << "Failed to build program.";
        qDebug() << QString::fromStdString(parseBuildReturnCode(err));
        qDebug() << buffer;

        return false;
    }

#ifndef MAKE_KERNEL
#define MAKE_KERNEL(var, name)\
    var = clCreateKernel(mProgram, name, &err);\
    if (!var || err != CL_SUCCESS)\
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
    clReleaseKernel(mJacobiKernel);
    clReleaseKernel(mAdvectKernel);
    clReleaseKernel(mDivergenceKernel);
    clReleaseKernel(mGradientKernel);
    clReleaseKernel(mAddScaledKernel);
    clReleaseKernel(mVelocityBoundaryKernel);
    clReleaseKernel(mPressureBoundaryKernel);

    clReleaseProgram(mProgram);

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
    cl_int err;

    cl_float betaInverse = 1.0 / beta;
    err  = clSetKernelArg(mJacobiKernel, 0, sizeof(cl_image), &input.image());
    err |= clSetKernelArg(mJacobiKernel, 1, sizeof(cl_image), &b.image());
    err |= clSetKernelArg(mJacobiKernel, 2, sizeof(cl_image), &output.image());
    err |= clSetKernelArg(mJacobiKernel, 3, sizeof(cl_float), &alpha);
    err |= clSetKernelArg(mJacobiKernel, 4, sizeof(cl_float), &betaInverse);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed to set arguments for Jacobi kernel.";
        return false;
    }


    err = run2DKernel(mCLWrapper->queue(), mCLWrapper->device(), mJacobiKernel, output.width(), output.height());

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed enqueuing Jacobi kernel.";
        qDebug() << QString::fromStdString(parseEnqueueKernelReturnCode(err));
        return false;
    }

    return true;
}

bool Fluid2DSimulationCLProgram::advect(MyCLImage2D &quantity,
                                        MyCLImage2D &velocity,
                                        MyCLImage2D &output,
                                        cl_float dt,
                                        cl_float gridSize)
{
    cl_int err;

    cl_float dt_h = dt / gridSize;
    err  = clSetKernelArg(mAdvectKernel, 0, sizeof(cl_image), &quantity.image());
    err |= clSetKernelArg(mAdvectKernel, 1, sizeof(cl_image), &velocity.image());
    err |= clSetKernelArg(mAdvectKernel, 2, sizeof(cl_image), &output.image());
    err |= clSetKernelArg(mAdvectKernel, 3, sizeof(cl_float), &dt_h);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed setting arguments for advection kernel.";
        return false;
    }


    err = run2DKernel(mCLWrapper->queue(), mCLWrapper->device(), mAdvectKernel, output.width(), output.height());

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed enqueuing advection kernel.";
        qDebug() << QString::fromStdString(parseEnqueueKernelReturnCode(err));
        return false;
    }

    return true;
}

bool Fluid2DSimulationCLProgram::divergence(MyCLImage2D &vecField,
                                            MyCLImage2D &output,
                                            cl_float gridSize)
{
    cl_int err;

    cl_float hInv = 1 / gridSize;
    err  = clSetKernelArg(mDivergenceKernel, 0, sizeof(cl_image), &vecField.image());
    err |= clSetKernelArg(mDivergenceKernel, 1, sizeof(cl_image), &output.image());
    err |= clSetKernelArg(mDivergenceKernel, 2, sizeof(cl_float), &hInv);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed setting arguments for divergence kernel.";
        return false;
    }

    err = run2DKernel(mCLWrapper->queue(), mCLWrapper->device(), mDivergenceKernel, output.width(), output.height());

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed enqueuing divergence kernel.";
        qDebug() << QString::fromStdString(parseEnqueueKernelReturnCode(err));
        return false;
    }

    return true;
}


bool Fluid2DSimulationCLProgram::gradient(MyCLImage2D &func,
                                          MyCLImage2D &output,
                                          cl_float gridSize)
{
    cl_int err;

    cl_float hInv = 1 / gridSize;
    err  = clSetKernelArg(mGradientKernel, 0, sizeof(cl_image), &func.image());
    err |= clSetKernelArg(mGradientKernel, 1, sizeof(cl_image), &output.image());
    err |= clSetKernelArg(mGradientKernel, 2, sizeof(cl_float), &hInv);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed setting arguments for gradient kernel.";
        return false;
    }

    err = run2DKernel(mCLWrapper->queue(), mCLWrapper->device(), mGradientKernel, output.width(), output.height());

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed enqueuing gradient kernel.";
        qDebug() << QString::fromStdString(parseEnqueueKernelReturnCode(err));
        return false;
    }

    return true;
}

bool Fluid2DSimulationCLProgram::addScaled(MyCLImage2D &t1, MyCLImage2D &t2,
                                           cl_float multiplier,
                                           MyCLImage2D &sum)
{
    cl_int err;

    err  = clSetKernelArg(mAddScaledKernel, 0, sizeof(cl_image), &t1.image());
    err |= clSetKernelArg(mAddScaledKernel, 1, sizeof(cl_image), &t2.image());
    err |= clSetKernelArg(mAddScaledKernel, 2, sizeof(cl_float), &multiplier);
    err |= clSetKernelArg(mAddScaledKernel, 3, sizeof(cl_image), &sum.image());

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed setting arguments for addScaled kernel.";
        return false;
    }

    err = run2DKernel(mCLWrapper->queue(), mCLWrapper->device(), mAddScaledKernel, sum.width(), sum.height());

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed enqueuing addScaled kernel.";
        qDebug() << QString::fromStdString(parseEnqueueKernelReturnCode(err));
        return false;
    }

    return true;
}


bool Fluid2DSimulationCLProgram::velocityBoundary(MyCLImage2D &img, MyCLImage2D &out)
{
    cl_int err;

    err  = clSetKernelArg(mVelocityBoundaryKernel, 0, sizeof(cl_image), &img.image());
    err |= clSetKernelArg(mVelocityBoundaryKernel, 1, sizeof(cl_image), &out.image());

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed setting arguments for velocityBoundary kernel.";
        return false;
    }

    err = run2DKernel(mCLWrapper->queue(), mCLWrapper->device(), mVelocityBoundaryKernel, out.width(), out.height());

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed enqueuing velocityBoundary kernel.";
        qDebug() << QString::fromStdString(parseEnqueueKernelReturnCode(err));
        return false;
    }

    return true;
}

bool Fluid2DSimulationCLProgram::pressureBoundary(MyCLImage2D &img, MyCLImage2D &out)
{
    cl_int err;

    err  = clSetKernelArg(mPressureBoundaryKernel, 0, sizeof(cl_image), &img.image());
    err |= clSetKernelArg(mPressureBoundaryKernel, 1, sizeof(cl_image), &out.image());

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed setting arguments for pressureBoundary kernel.";
        return false;
    }

    err = run2DKernel(mCLWrapper->queue(), mCLWrapper->device(), mPressureBoundaryKernel, out.width(), out.height());

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed enqueuing pressureBoundary kernel.";
        qDebug() << QString::fromStdString(parseEnqueueKernelReturnCode(err));
        return false;
    }

    return true;
}

/// Gets the source code for the program.
static QByteArray getSource()
{
    QFile sourceFile(":/compute/fluidSimulation.cl");
    Q_ASSERT( sourceFile.exists() ); // The file must exist if the project is set up correctly.

    if (!sourceFile.open(QIODevice::ReadOnly))
    {
        // The file must also be open-able.
        qFatal("Could not open file.");
    }

    QString sourceCode = QTextStream(&sourceFile).readAll();
    sourceFile.close();

    QByteArray bytes = sourceCode.toLatin1();
    return bytes;
}

/// Finds the next multiple of `factor` greater than
/// or equal to `startVal`.
static size_t nextMultiple(size_t startVal, size_t factor)
{
    size_t remainder = startVal % factor;

    if (remainder > 0)
        return startVal + (factor - remainder);
    else
        return startVal;
}

/// Factors num into f1 * f2 such that f1 and f2 are
/// close to each other.
static void factorEvenly(size_t num, size_t *f1, size_t *f2)
{
    /* Strategy:
        Write out the prime factorization of num in ascending order.
        Every (2k)th factor goes to f1, every (2k+1)th factor goes to f2. */

    const size_t initialNum = num;
    *f1 = 1;
    *f2 = 1;

    size_t *cur = f1;
    size_t *next = f2;

    size_t current_factor = 2;

    while (num > 1)
    {
        // If num is divisible by current_factor...
        if (num % current_factor == 0)
        {
            // Then multiply that factor into cur
            *cur *= current_factor;

            // Swap cur with next
            size_t *temp = cur;
            cur = next;
            next = temp;

            // Divide num.
            num /= current_factor;
        }
        else
        {
            // Go to the next factor.
            ++current_factor;
        }
    }

    Q_ASSERT( (*f1) * (*f2) == initialNum);
}

static cl_int run2DKernel(cl_command_queue queue, cl_device_id device, cl_kernel kernel, size_t globalSize1, size_t globalSize2)
{
    cl_int err;

    size_t idealWorkGroupSize;
    err = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &idealWorkGroupSize, NULL);

    if (err != CL_SUCCESS)
        return err;


    size_t localSize1, localSize2;
    factorEvenly(idealWorkGroupSize, &localSize1, &localSize2);

    globalSize1 = nextMultiple(globalSize1, localSize1);
    globalSize2 = nextMultiple(globalSize2, localSize2);

    size_t globals[2] = {globalSize1, globalSize2};
    size_t locals[2] = {localSize1, localSize2};

    return clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globals, locals, 0, NULL, NULL);
}
