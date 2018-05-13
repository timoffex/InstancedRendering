#include "grasswindclprogram.h"

#include <QFile>
#include <QString>
#include <QTextStream>
#include <QByteArray>
#include <QDebug>



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
        qDebug() << buffer;

        return false;
    }


    mGrassReactKernel = clCreateKernel(mProgram, "reactToWind", &err);
    if (!mGrassReactKernel || err != CL_SUCCESS)
    {
        qDebug() << "Failed to create kernel.";
        return false;
    }


    mCreated = true;
    return true;
}


bool GrassWindCLProgram::reactToWind(cl_mem grassWindPositions,
                                     cl_mem grassWindVelocities,
                                     cl_mem grassWindStrength,
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
    err |= clSetKernelArg(mGrassReactKernel, 2, sizeof(cl_mem), &grassWindStrength);
    err |= clSetKernelArg(mGrassReactKernel, 3, sizeof(cl_uint), &numBlades);
    err |= clSetKernelArg(mGrassReactKernel, 4, sizeof(cl_float), &dt);

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

    size_t globalSize = numBlades;
    if (globalSize % workGroupSize > 0)
        globalSize += workGroupSize - (globalSize % workGroupSize); // global size must be divisible by work group size

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
