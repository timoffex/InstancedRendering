#include "grasswindclprogram.h"

#include "cl_interface/myclerrors.h"

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

    if (globalSize1 % localSize1 > 0)
        globalSize1 += localSize1 - (globalSize1 % localSize1); // global size must be divisible by work group size

    if (globalSize2 % localSize2 > 0)
        globalSize2 += localSize2 - (globalSize2 % localSize2); // global size must be divisible by work group size

    size_t globals[2] = {globalSize1, globalSize2};
    size_t locals[2] = {localSize1, localSize2};

    return clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globals, locals, 0, NULL, NULL);
}


GrassWindCLProgram::GrassWindCLProgram()
    : mCreated(false)
{
}

bool GrassWindCLProgram::create(MyCLWrapper *wrapper)
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
        qDebug() << QString::fromStdString(parseBuildReturnCode(err));
        qDebug() << buffer;

        return false;
    }

    mGrassReact2Kernel = clCreateKernel(mProgram, "reactToWind2", &err);
    if (!mGrassReact2Kernel || err != CL_SUCCESS)
    {
        qDebug() << "Failed to create reactToWind2 kernel.";
        return false;
    }

    mCreated = true;
    return true;
}

void GrassWindCLProgram::release()
{
    clReleaseKernel(mGrassReact2Kernel);

    clReleaseProgram(mProgram);

    mCreated = false;
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
        qDebug() << QString::fromStdString(parseEnqueueKernelReturnCode(err));
        return false;
    }

    return true;
}
