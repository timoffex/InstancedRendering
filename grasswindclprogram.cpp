#include "grasswindclprogram.h"

#include <QFile>
#include <QString>
#include <QTextStream>
#include <QByteArray>
#include <QDebug>

#include <QtMath>


/// Gets the source code for the program.
static QByteArray getSource()
{
    QFile sourceFile(":/compute/grassWindReact.cl");
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

/// Prints out the error code to qDebug().
static void parseEnqueueKernelReturnCode(cl_int err)
{
#ifndef CASE // Avoid accidentally overwriting a macro defined elsewhere.
#define CASE(name) case name: qDebug() << #name; break;
    switch (err)
    {
    case CL_SUCCESS: qDebug() << "Enqueue-kernel was successful."; break;
    CASE(CL_INVALID_PROGRAM_EXECUTABLE);
    CASE(CL_INVALID_COMMAND_QUEUE);
    CASE(CL_INVALID_KERNEL);
    CASE(CL_INVALID_CONTEXT);
    CASE(CL_INVALID_KERNEL_ARGS);
    CASE(CL_INVALID_WORK_DIMENSION);
    CASE(CL_INVALID_WORK_GROUP_SIZE);
    CASE(CL_INVALID_WORK_ITEM_SIZE);
    CASE(CL_INVALID_GLOBAL_OFFSET);
    CASE(CL_OUT_OF_RESOURCES);
    CASE(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    CASE(CL_INVALID_EVENT_WAIT_LIST);
    CASE(CL_OUT_OF_HOST_MEMORY);

    default:
        qDebug() << "Unknown error.";
    }
#undef CASE
#endif
}

static void parseBuildReturnCode(cl_int err)
{
#ifndef CASE // Avoid accidentally overwriting a macro defined elsewhere.
#define CASE(name) case name: qDebug() << #name; break;
    switch (err)
    {
    case CL_SUCCESS: qDebug() << "Program build was successful"; break;
    CASE(CL_INVALID_PROGRAM);
    CASE(CL_INVALID_VALUE);
    CASE(CL_INVALID_DEVICE);
    CASE(CL_INVALID_BINARY);
    CASE(CL_INVALID_BUILD_OPTIONS);
    CASE(CL_INVALID_OPERATION);
    CASE(CL_COMPILER_NOT_AVAILABLE);
    CASE(CL_BUILD_PROGRAM_FAILURE);
    CASE(CL_OUT_OF_HOST_MEMORY);
    default:
        qDebug() << "Unknown error.";
    }
#undef CASE
#endif
}

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

    if (globalSize1 % localSize1 > 0)
        globalSize1 += localSize1 - (globalSize1 % localSize1); // global size must be divisible by work group size

    if (globalSize2 % localSize2 > 0)
        globalSize2 += localSize2 - (globalSize2 % localSize2); // global size must be divisible by work group size

    size_t globals[2] = {globalSize1, globalSize2};
    size_t locals[2] = {localSize1, localSize2};

    return clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globals, locals, 0, NULL, NULL);
}


GrassWindCLProgram::GrassWindCLProgram()
{
    mCreated = false;
}

bool GrassWindCLProgram::create(MyCLWrapper *wrapper)
{
    /* Steps:
        1) Create a program using clCreateProgramWithSource()
        2) Build the program using clBuildProgram()
        3) Create the kernel with clCreateKernel() (a program may
            contain multiple kernels)
    */

    mCLWrapper = wrapper;

    QByteArray sourceCode = getSource();
    const char *data = sourceCode.data();

    cl_int err;

    mProgram = clCreateProgramWithSource(wrapper->context(), 1, &data, NULL, &err);

    if (!mProgram || err != CL_SUCCESS) // TODO: example code doesn't check err
    {
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
        parseBuildReturnCode(err);
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

    MAKE_KERNEL(mGrassReactKernel, "reactToWind");
    MAKE_KERNEL(mGrassReact2Kernel, "reactToWind2");
    MAKE_KERNEL(mUpdateWindKernel, "updateWind");
    MAKE_KERNEL(mZeroTextureKernel, "zeroTexture");

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


bool GrassWindCLProgram::reactToWind(cl_mem grassWindPositions,
                                     cl_mem grassWindVelocities,
                                     cl_mem grassNormalizedOffsets,
                                     cl_image windStrengthImage,
                                     cl_uint numBlades,
                                     cl_float dt)
{
    Q_ASSERT( mCreated ); // a guard against stupidity


    /* Steps:
        1) Set kernel arguments.
        2) Get an ideal work group size. A GPU usually consists of several cores
            each having several SIMD units. If our work group size is too small,
            we won't utilize the SIMD units very well.
        3) Enqueue the kernel.
    */

    cl_int err = 0;
    err  = clSetKernelArg(mGrassReactKernel, 0, sizeof(cl_mem), &grassWindPositions);
    err |= clSetKernelArg(mGrassReactKernel, 1, sizeof(cl_mem), &grassWindVelocities);
    err |= clSetKernelArg(mGrassReactKernel, 2, sizeof(cl_mem), &grassNormalizedOffsets);
    err |= clSetKernelArg(mGrassReactKernel, 3, sizeof(cl_image), &windStrengthImage);
    err |= clSetKernelArg(mGrassReactKernel, 4, sizeof(cl_uint), &numBlades);
    err |= clSetKernelArg(mGrassReactKernel, 5, sizeof(cl_float), &dt);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failure setting some arguments.";
        return false;
    }

    size_t workGroupSize;
    err = clGetKernelWorkGroupInfo(mGrassReactKernel,
                                   mCLWrapper->device(),
                                   CL_KERNEL_WORK_GROUP_SIZE,
                                   sizeof(workGroupSize), &workGroupSize, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed to get work group size.";
        return false;
    }

    size_t globalSize = nextMultiple(numBlades, workGroupSize);
    err = clEnqueueNDRangeKernel(mCLWrapper->queue(), mGrassReactKernel, 1, NULL, &globalSize, &workGroupSize, 0, NULL, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed to enqueue kernel.";
        qDebug() << "Work group size: " << workGroupSize;
        qDebug() << "Global size: " << globalSize;
        parseEnqueueKernelReturnCode(err);
        return false;
    }

    return true;
}

bool GrassWindCLProgram::reactToWind2(cl_mem grassWindOffsets,
                                      cl_mem grassPeriodOffsets,
                                      cl_mem grassNormalizedPositions,
                                      cl_image windVelocity,
                                      cl_uint numBlades,
                                      cl_float time)
{
    Q_ASSERT( mCreated );

    cl_int err;

    err  = clSetKernelArg(mGrassReact2Kernel, 0, sizeof(cl_mem), &grassWindOffsets);
    err |= clSetKernelArg(mGrassReact2Kernel, 1, sizeof(cl_mem), &grassPeriodOffsets);
    err |= clSetKernelArg(mGrassReact2Kernel, 2, sizeof(cl_mem), &grassNormalizedPositions);
    err |= clSetKernelArg(mGrassReact2Kernel, 3, sizeof(cl_image), &windVelocity);
    err |= clSetKernelArg(mGrassReact2Kernel, 4, sizeof(cl_uint), &numBlades);
    err |= clSetKernelArg(mGrassReact2Kernel, 5, sizeof(cl_float), &time);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed to set kernel arguments for the second grass react.";
        return false;
    }

    size_t localSize;
    err = clGetKernelWorkGroupInfo(mGrassReact2Kernel, mCLWrapper->device(), CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &localSize, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed to retrieve work group size for mGrassReact2Kernel";
        return false;
    }

    size_t globalSize = nextMultiple(numBlades, localSize);
    err = clEnqueueNDRangeKernel(mCLWrapper->queue(), mGrassReact2Kernel, 1, NULL, &globalSize, &localSize, 0, NULL, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed to enqueue mGrassReact2Kernel.";
        parseEnqueueKernelReturnCode(err);
        return false;
    }

    return true;
}

bool GrassWindCLProgram::updateWind(cl_image windSpeeds,
                                    cl_image forces,
                                    cl_float gridSize,
                                    cl_float dt,
                                    cl_float viscosity,
                                    cl_image output,
                                    unsigned int imageWidth,
                                    unsigned int imageHeight)
{
    Q_ASSERT( mCreated );

    cl_float gridSizeInv = 1.0 / gridSize;

    cl_int err = 0;
    err  = clSetKernelArg(mUpdateWindKernel, 0, sizeof(cl_image), &windSpeeds);
    err |= clSetKernelArg(mUpdateWindKernel, 1, sizeof(cl_image), &forces);
    err |= clSetKernelArg(mUpdateWindKernel, 2, sizeof(cl_image), &output);
    err |= clSetKernelArg(mUpdateWindKernel, 3, sizeof(cl_float), &gridSizeInv);
    err |= clSetKernelArg(mUpdateWindKernel, 4, sizeof(cl_float), &dt);
    err |= clSetKernelArg(mUpdateWindKernel, 5, sizeof(cl_float), &viscosity);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failure setting some arguments in updateWind kernel.";
        return false;
    }

    size_t workGroupSize;
    err = clGetKernelWorkGroupInfo(mUpdateWindKernel,
                                   mCLWrapper->device(),
                                   CL_KERNEL_WORK_GROUP_SIZE,
                                   sizeof(workGroupSize), &workGroupSize, NULL);
    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed to query ideal work group size.";
        return false;
    }


    size_t workGroupSize1;
    size_t workGroupSize2;
    factorEvenly(workGroupSize, &workGroupSize1, &workGroupSize2);


    size_t globalSize1 = imageWidth;
    if (globalSize1 % workGroupSize1 > 0)
        globalSize1 += workGroupSize1 - (globalSize1 % workGroupSize1); // global size must be divisible by work group size

    size_t globalSize2 = imageHeight;
    if (globalSize2 % workGroupSize2 > 0)
        globalSize2 += workGroupSize2 - (globalSize2 % workGroupSize2); // global size must be divisible by work group size

    size_t globals[2] = {globalSize1, globalSize2};
    size_t locals[2] = {workGroupSize1, workGroupSize2};

    err = clEnqueueNDRangeKernel(mCLWrapper->queue(), mUpdateWindKernel, 2, NULL, globals, locals, 0, NULL, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed to enqueue updateWind kernel.";
        qDebug() << "Work group sizes: (" << workGroupSize1 << ", " << workGroupSize2 << ")";
        qDebug() << "Global sizes: (" << globalSize1 << ", " << globalSize2 << ")";
        parseEnqueueKernelReturnCode(err);
        return false;
    }

    return true;
}

bool GrassWindCLProgram::updateWindNew(cl_image windSpeeds,
                                       cl_image forces,
                                       cl_float gridSize,
                                       cl_float dt,
                                       cl_float density,
                                       cl_float viscosity,
                                       cl_image pressure,
                                       cl_image temp1,
                                       cl_image temp2)
{
    cl_int err;

    size_t width, height;
    err  = clGetImageInfo(windSpeeds, CL_IMAGE_WIDTH, sizeof(size_t), &width, NULL);
    err |= clGetImageInfo(windSpeeds, CL_IMAGE_HEIGHT, sizeof(size_t), &height, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed to query windSpeeds width and height.";
        return false;
    }

    /* Algorithm:
        1) temp1 <- advect windSpeeds
        2) temp1 <- diffuse temp1
            i)   jacobi(temp1, temp1, temp2, hh/dt, 1/(4 + hh/dt))
            ii)  jacobi(temp2, temp2, temp1, hh/dt, 1/(4 + hh/dt))
            iii) repeat several times
        3) windSpeeds <- temp1 + forces * scale
        4) compute pressure
            i)   temp2 <- divergence of windSpeeds
            ii)  jacobi(pressure, temp2, temp1, -hh, 4)
            iii) jacobi(temp1, temp2, pressure, -hh, 4)
            iv)  repeat (ii)-(iii) several times
        5) subtract gradient of pressure from windSpeeds
            i)   temp2 <- gradient of pressure
            ii)  temp1 <- windSpeeds + temp2 * -1 * scale
        6) enforce boundary conditions
     * */

    /* STEP (1) => temp1 will contain advected wind speeds */
    if (!advect(windSpeeds, windSpeeds, temp1, dt, gridSize))
    {
        qDebug() << "Failure in step 1 of wind update.";
        return false;
    }


    /* STEP (2) => temp1 will contain diffused wind speeds*/
    cl_float hh_vdt = gridSize * gridSize / (viscosity * dt);
    for (int iteration = 0; iteration < 30; ++iteration)
    {
        cl_image *t1 = &temp1;
        cl_image *t2 = &temp2;

        for (int subIteration = 0; subIteration < 2; ++subIteration)
        {
            if (!jacobi(*t1, *t1, *t2, hh_vdt, 4 + hh_vdt))
            {
                qDebug() << "Failure in step 2i of wind update.";
                return false;
            }

            // Swap pointers.
            cl_image *tt = t1;
            t1 = t2;
            t2 = tt;
        }
    }


    /* STEP (3) => windSpeeds will contain temp1 + forces * scale */
    if (!addScaled(temp1, forces, dt, windSpeeds))
    {
        qDebug() << "Failure in step 3 of wind update.";
        return false;
    }


    /* STEP (4) =>
            temp2 will contain the divergence of windSpeeds;
            windSpeeds same as before
            pressure will be updated
    */
    if (!divergence(windSpeeds, temp2, gridSize))
    {
        qDebug() << "Failure in step 4i of wind update.";
        return false;
    }

    // NOTE: Doing too many of these iterations makes everything explode
    // into tiny little pressure waves.
    for (int iteration = 0; iteration < 4; ++iteration)
    {
        cl_image *t1 = &pressure;
        cl_image *t2 = &temp1;

        for (int subIteration = 0; subIteration < 2; ++subIteration)
        {
            if (!jacobi(*t1, temp2, *t2, -gridSize * gridSize, 4))
            {
                qDebug() << "Failure in step 4ii of wind update.";
                return false;
            }

            // Swap pointers.
            cl_image *tt = t1;
            t1 = t2;
            t2 = tt;
        }
    }

    /* STEP (5) =>
            temp2 will contain gradient of pressure
            temp1 will contain windSpeeds - gradient of pressure
            pressure still the same
    */
    if (!gradient(pressure, temp2, gridSize))
    {
        qDebug() << "Failure in step 5i of wind update.";
        return false;
    }

    if (!addScaled(windSpeeds, temp2, -1.0/density, temp1))
    {
        qDebug() << "Failure in step 5ii of wind update.";
        return false;
    }


    /* STEP (6) enforce boundary conditions */
//    addScaled(temp1, temp2, 0, windSpeeds);
    if (!velocityBoundary(temp1, windSpeeds))
    {
        qDebug() << "Failure running velocityBoundary.";
        return false;
    }

    if (!pressureBoundary(pressure, temp1))
    {
        qDebug() << "Failure running pressureBoundary.";
        return false;
    }

    if (!addScaled(temp1, temp1, 0, pressure))
    {
        qDebug() << "Failure running addScaled.";
        return false;
    }

    return true;
}

bool GrassWindCLProgram::zeroTexture(cl_image img)
{
    cl_int err;

    size_t width, height;
    err  = clGetImageInfo(img, CL_IMAGE_WIDTH, sizeof(size_t), &width, NULL);
    err |= clGetImageInfo(img, CL_IMAGE_HEIGHT, sizeof(size_t), &height, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed to query image for zeroTexture kernel.";
        return false;
    }


    size_t idealWorkGroup;
    err = clGetKernelWorkGroupInfo(mZeroTextureKernel,
                                   mCLWrapper->device(),
                                   CL_KERNEL_WORK_GROUP_SIZE,
                                   sizeof(idealWorkGroup),
                                   &idealWorkGroup,
                                   NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed to query ideal work group size for zeroTexture kernel.";
        return false;
    }

    size_t localSize1, localSize2;
    factorEvenly(idealWorkGroup, &localSize1, &localSize2);

    size_t globalSize1 = width;
    size_t globalSize2 = height;

    if (globalSize1 % localSize1 > 0)
        globalSize1 += localSize1 - (globalSize1 % localSize1); // global size must be divisible by work group size

    if (globalSize2 % localSize2 > 0)
        globalSize2 += localSize2 - (globalSize2 % localSize2); // global size must be divisible by work group size


    err = clSetKernelArg(mZeroTextureKernel, 0, sizeof(cl_image), &img);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed to set kernel arg for zeroTexture kernel.";
        return false;
    }

    size_t globals[2] = {globalSize1, globalSize2};
    size_t locals[2] = {localSize1, localSize2};

    err = clEnqueueNDRangeKernel(mCLWrapper->queue(), mZeroTextureKernel, 2, NULL, globals, locals, 0, NULL, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed to enqueue zeroTexture kernel.";
        parseEnqueueKernelReturnCode(err);
        return false;
    }

    return true;
}


bool GrassWindCLProgram::jacobi(cl_image input,
                                cl_image b,
                                cl_image output,
                                cl_float alpha,
                                cl_float beta)
{
    cl_int err;

    size_t imWidth, imHeight;
    err  = clGetImageInfo(output, CL_IMAGE_WIDTH, sizeof(size_t), &imWidth, NULL);
    err |= clGetImageInfo(output, CL_IMAGE_HEIGHT, sizeof(size_t), &imHeight, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed querying output image.";
        return false;
    }

    cl_float betaInverse = 1.0 / beta;
    err  = clSetKernelArg(mJacobiKernel, 0, sizeof(cl_image), &input);
    err |= clSetKernelArg(mJacobiKernel, 1, sizeof(cl_image), &b);
    err |= clSetKernelArg(mJacobiKernel, 2, sizeof(cl_image), &output);
    err |= clSetKernelArg(mJacobiKernel, 3, sizeof(cl_float), &alpha);
    err |= clSetKernelArg(mJacobiKernel, 4, sizeof(cl_float), &betaInverse);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed to set arguments for Jacobi kernel.";
        return false;
    }


    err = run2DKernel(mCLWrapper->queue(), mCLWrapper->device(), mJacobiKernel, imWidth, imHeight);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed enqueuing Jacobi kernel.";
        parseEnqueueKernelReturnCode(err);
        return false;
    }

    return true;
}

bool GrassWindCLProgram::advect(cl_image quantity,
                                cl_image velocity,
                                cl_image output,
                                cl_float dt,
                                cl_float gridSize)
{
    cl_int err;

    size_t imWidth, imHeight;
    err  = clGetImageInfo(output, CL_IMAGE_WIDTH, sizeof(size_t), &imWidth, NULL);
    err |= clGetImageInfo(output, CL_IMAGE_HEIGHT, sizeof(size_t), &imHeight, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed querying output image.";
        return false;
    }

    cl_float dt_h = dt / gridSize;
    err  = clSetKernelArg(mAdvectKernel, 0, sizeof(cl_image), &quantity);
    err |= clSetKernelArg(mAdvectKernel, 1, sizeof(cl_image), &velocity);
    err |= clSetKernelArg(mAdvectKernel, 2, sizeof(cl_image), &output);
    err |= clSetKernelArg(mAdvectKernel, 3, sizeof(cl_image), &dt_h);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed setting arguments for advection kernel.";
        return false;
    }


    err = run2DKernel(mCLWrapper->queue(), mCLWrapper->device(), mAdvectKernel, imWidth, imHeight);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed enqueuing advection kernel.";
        parseEnqueueKernelReturnCode(err);
        return false;
    }

    return true;
}

bool GrassWindCLProgram::divergence(cl_image vecField,
                                    cl_image output,
                                    cl_float gridSize)
{
    cl_int err;

    size_t imWidth, imHeight;
    err  = clGetImageInfo(output, CL_IMAGE_WIDTH, sizeof(size_t), &imWidth, NULL);
    err |= clGetImageInfo(output, CL_IMAGE_HEIGHT, sizeof(size_t), &imHeight, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed querying output image.";
        return false;
    }

    cl_float hInv = 1 / gridSize;
    err  = clSetKernelArg(mDivergenceKernel, 0, sizeof(cl_image), &vecField);
    err |= clSetKernelArg(mDivergenceKernel, 1, sizeof(cl_image), &output);
    err |= clSetKernelArg(mDivergenceKernel, 2, sizeof(cl_float), &hInv);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed setting arguments for divergence kernel.";
        return false;
    }

    err = run2DKernel(mCLWrapper->queue(), mCLWrapper->device(), mDivergenceKernel, imWidth, imHeight);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed enqueuing divergence kernel.";
        parseEnqueueKernelReturnCode(err);
        return false;
    }

    return true;
}


bool GrassWindCLProgram::gradient(cl_image func,
                                  cl_image output,
                                  cl_float gridSize)
{
    cl_int err;

    size_t imWidth, imHeight;
    err  = clGetImageInfo(output, CL_IMAGE_WIDTH, sizeof(size_t), &imWidth, NULL);
    err |= clGetImageInfo(output, CL_IMAGE_HEIGHT, sizeof(size_t), &imHeight, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed querying output image.";
        return false;
    }

    cl_float hInv = 1 / gridSize;
    err  = clSetKernelArg(mGradientKernel, 0, sizeof(cl_image), &func);
    err |= clSetKernelArg(mGradientKernel, 1, sizeof(cl_image), &output);
    err |= clSetKernelArg(mGradientKernel, 2, sizeof(cl_float), &hInv);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed setting arguments for gradient kernel.";
        return false;
    }

    err = run2DKernel(mCLWrapper->queue(), mCLWrapper->device(), mGradientKernel, imWidth, imHeight);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed enqueuing gradient kernel.";
        parseEnqueueKernelReturnCode(err);
        return false;
    }

    return true;
}

bool GrassWindCLProgram::addScaled(cl_image t1, cl_image t2,
                                   cl_float multiplier,
                                   cl_image sum)
{
    cl_int err;

    size_t imWidth, imHeight;
    err  = clGetImageInfo(sum, CL_IMAGE_WIDTH, sizeof(size_t), &imWidth, NULL);
    err |= clGetImageInfo(sum, CL_IMAGE_HEIGHT, sizeof(size_t), &imHeight, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed querying output image.";
        return false;
    }

    err  = clSetKernelArg(mAddScaledKernel, 0, sizeof(cl_image), &t1);
    err |= clSetKernelArg(mAddScaledKernel, 1, sizeof(cl_image), &t2);
    err |= clSetKernelArg(mAddScaledKernel, 2, sizeof(cl_float), &multiplier);
    err |= clSetKernelArg(mAddScaledKernel, 3, sizeof(cl_image), &sum);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed setting arguments for addScaled kernel.";
        return false;
    }

    err = run2DKernel(mCLWrapper->queue(), mCLWrapper->device(), mAddScaledKernel, imWidth, imHeight);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed enqueuing addScaled kernel.";
        parseEnqueueKernelReturnCode(err);
        return false;
    }

    return true;
}


bool GrassWindCLProgram::velocityBoundary(cl_image img, cl_image out)
{
    cl_int err;

    size_t imWidth, imHeight;
    err  = clGetImageInfo(out, CL_IMAGE_WIDTH, sizeof(size_t), &imWidth, NULL);
    err |= clGetImageInfo(out, CL_IMAGE_HEIGHT, sizeof(size_t), &imHeight, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed querying output image.";
        return false;
    }

    err  = clSetKernelArg(mVelocityBoundaryKernel, 0, sizeof(cl_image), &img);
    err |= clSetKernelArg(mVelocityBoundaryKernel, 1, sizeof(cl_image), &out);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed setting arguments for velocityBoundary kernel.";
        return false;
    }

    err = run2DKernel(mCLWrapper->queue(), mCLWrapper->device(), mVelocityBoundaryKernel, imWidth, imHeight);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed enqueuing velocityBoundary kernel.";
        parseEnqueueKernelReturnCode(err);
        return false;
    }

    return true;
}

bool GrassWindCLProgram::pressureBoundary(cl_image img, cl_image out)
{
    cl_int err;

    size_t imWidth, imHeight;
    err  = clGetImageInfo(out, CL_IMAGE_WIDTH, sizeof(size_t), &imWidth, NULL);
    err |= clGetImageInfo(out, CL_IMAGE_HEIGHT, sizeof(size_t), &imHeight, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed querying output image.";
        return false;
    }

    err  = clSetKernelArg(mPressureBoundaryKernel, 0, sizeof(cl_image), &img);
    err |= clSetKernelArg(mPressureBoundaryKernel, 1, sizeof(cl_image), &out);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed setting arguments for pressureBoundary kernel.";
        return false;
    }

    err = run2DKernel(mCLWrapper->queue(), mCLWrapper->device(), mPressureBoundaryKernel, imWidth, imHeight);

    if (err != CL_SUCCESS)
    {
        qDebug() << "Failed enqueuing pressureBoundary kernel.";
        parseEnqueueKernelReturnCode(err);
        return false;
    }

    return true;
}
