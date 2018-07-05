#ifndef MYCLKERNEL_H
#define MYCLKERNEL_H

#include "include_opencl.h"
#include "myclimage.h"
#include "myclwrapper.h"
#include "myclerrors.h"

#include <string>
#include <type_traits>

#include <QDebug>

/// A wrapper for an OpenCL kernel. The template arguments are the types
/// of the kernel arguments, which may be any valid cl_* type or MyCLImage2D &.
/// It is very important that MyCLImage2D is passed by reference!
///
/// The MyCLImage2D thing will be removed and MyCLImage2D will become
/// implicitly convertible to cl_image.
template< typename FirstType, typename ... OtherTypes >
class MyCLKernel
{
public:

    MyCLKernel() : mCreated(false) {}

    ~MyCLKernel()
    {
        destroy();
    }

    /// Creates the kernel from the program.
    bool createFromProgram(MyCLWrapper *wrapper, cl_program program, const char *name)
    {
        mCLWrapper = wrapper;
        cl_int err;

        mKernel = clCreateKernel(program, name, &err);
        if (!mKernel || err != CL_SUCCESS)
        {
            qDebug() << "Failed to create " << name << " kernel.";
            return false;
        }

        mCreated = true;
        return true;
    }

    /// Destroys the kernel.
    void destroy()
    {
        if (mCreated)
        {
            clReleaseKernel(mKernel);
            mCreated = false;
        }
    }

    /// Invokes the kernel with the given arguments and a 1-dimensional layout.
    bool operator() (size_t globalSize, FirstType firstArg, OtherTypes ... restArgs)
    {
        Q_ASSERT(mCreated);

        if (!setKernelArg<FirstType, OtherTypes...>(0, firstArg, restArgs...))
            return false;

        cl_int err;

        size_t idealWorkGroupSize;
        err = clGetKernelWorkGroupInfo(mKernel, mCLWrapper->device(), CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &idealWorkGroupSize, NULL);

        if (err != CL_SUCCESS)
        {
            qDebug() << "Couldn't get kernel work group size."; // TODO print name
            return false;
        }

        globalSize = nextMultiple(globalSize, idealWorkGroupSize);

        err = clEnqueueNDRangeKernel(mCLWrapper->queue(), mKernel, 1, NULL, &globalSize, &idealWorkGroupSize, 0, NULL, NULL);

        if (err != CL_SUCCESS)
        {
            qDebug() << QString::fromStdString(parseEnqueueKernelReturnCode(err));
            return false;
        }

        return true;
    }


    /// Invokes the kernel with the given arguments and a 2-dimensional layout.
    bool operator() (size_t globalSize1, size_t globalSize2, FirstType firstArg, OtherTypes ... restArgs)
    {
        Q_ASSERT(mCreated);

        if (!setKernelArg<FirstType, OtherTypes...>(0, firstArg, restArgs...))
            return false;


        cl_int err;

        size_t idealWorkGroupSize;
        err = clGetKernelWorkGroupInfo(mKernel, mCLWrapper->device(), CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &idealWorkGroupSize, NULL);

        if (err != CL_SUCCESS)
        {
            qDebug() << "Couldn't get kernel work group size."; // TODO print name
            return false;
        }

        size_t localSize1, localSize2;
        factorEvenly(idealWorkGroupSize, &localSize1, &localSize2);

        globalSize1 = nextMultiple(globalSize1, localSize1);
        globalSize2 = nextMultiple(globalSize2, localSize2);

        size_t globals[2] = {globalSize1, globalSize2};
        size_t locals[2] = {localSize1, localSize2};

        err = clEnqueueNDRangeKernel(mCLWrapper->queue(), mKernel, 2, NULL, globals, locals, 0, NULL, NULL);

        if (err != CL_SUCCESS)
        {
            qDebug() << QString::fromStdString(parseEnqueueKernelReturnCode(err));

            if (err == CL_INVALID_WORK_GROUP_SIZE)
            {
                /*
                 * On some devices, 2D work group sizes are not possible because the maximum size
                 * of a work group in the second dimension is 1. The below code prints out some
                 * helpful information that will hopefully help you figure out what went wrong.
                 *
                 * I got this error when I used CL_DEVICE_TYPE_CPU.
                 * */

                size_t maxWorkGroupSize;
                size_t maxWorkItemDimensions = 0;
                clGetDeviceInfo(mCLWrapper->device(), CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &maxWorkGroupSize, NULL);
                clGetDeviceInfo(mCLWrapper->device(), CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(size_t), &maxWorkItemDimensions, NULL);

                size_t maxWorkItemSizes[maxWorkItemDimensions];
                std::fill(maxWorkItemSizes, maxWorkItemSizes + maxWorkItemDimensions, 0);

                clGetDeviceInfo(mCLWrapper->device(), CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t) * maxWorkItemDimensions, maxWorkItemSizes, NULL);



                qDebug() << "The failure happened in clEnqueueNDRangeKernel().";
                qDebug() << "The maximum work group size for this device is " << maxWorkGroupSize;
                qDebug() << "The ideal work group size is " << idealWorkGroupSize;
                qDebug() << "The work group size used is " << locals[0] << " x " << locals[1] << " = " << locals[0] * locals[1];


                qDebug() << "The maximum number of work item dimensions is " << maxWorkItemDimensions << ". The max work group sizes are...";
                for (size_t i = 0; i < maxWorkItemDimensions; ++i)
                    qDebug() << "   dimension " << i << ": " << maxWorkItemSizes[i];
            }

            return false;
        }

        return true;
    }

private:
    bool mCreated;


    cl_kernel mKernel;
    MyCLWrapper *mCLWrapper;


    /// Sets the nth kernel argument to be arg1, and then sets the rest.
    /// MyCLImage2D and cl_* types are valid here.
    template< typename FirstArg, typename ... RestArgs >
    bool setKernelArg(int n, FirstArg arg1, RestArgs ... argsRest)
    {
        cl_int err;

        if constexpr (std::is_same<typename std::remove_reference<FirstArg>::type, MyCLImage2D>::value)
        {
            static_assert(std::is_reference<FirstArg>::value, "MyCLImage2D arguments need to be passed by reference.");
            err = clSetKernelArg(mKernel, n, sizeof(cl_image), &arg1.image());
        }
        else
        {
            err = clSetKernelArg(mKernel, n, sizeof(arg1), &arg1);
        }

        if (err != CL_SUCCESS)
        {
            // TODO: Parse clSetKernelArg() error code.
            qDebug() << "Failed to set argument #" << n << " for kernel.";
            return false;
        }

        if constexpr (sizeof...(argsRest) > 0)
            return setKernelArg<RestArgs...>(n + 1, argsRest...);
        else
            return true;
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
};

#endif // MYCLKERNEL_H
