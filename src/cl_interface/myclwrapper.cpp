#include "myclwrapper.h"

#include <QDebug>
#include <QOpenGLContext>

MyCLWrapper::MyCLWrapper()
    : mCreated(false)
{
}



#ifdef __APPLE__
#include <OpenGL.h>
static cl_context makeCLGLContext_Apple(cl_device_id *device, cl_int *err)
{
    CGLContextObj cglContext = CGLGetCurrentContext();
    CGLShareGroupObj cglShareGroup = CGLGetShareGroup(cglContext);

    cl_context_properties properties[] = {
        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
        (cl_context_properties)cglShareGroup, 0
    };

    return clCreateContext(properties, 1, device, NULL, NULL, err);
}
#endif

/// Creates an OpenCL context for sharing with the current OpenGL context.
/// An OpenGL context must be current.
static cl_context makeCLGLContext(cl_device_id *device, cl_int *err)
{
    /* Unfortunately, this code differs between platforms. */
#ifdef __APPLE__
    return makeCLGLContext_Apple(device, err);
#endif
}


bool MyCLWrapper::createFromGLContext(cl_device_type deviceType)
{
    // Necessary for creating a shared context.
    Q_ASSERT( QOpenGLContext::currentContext() != nullptr );

    cl_int err;


    // Get the desired device.
    err = clGetDeviceIDs(NULL, deviceType, 1, &mDevice, NULL);

    if (err != CL_SUCCESS)
        return false;


    // Create the context.
    mContext = makeCLGLContext(&mDevice, &err);

    if (err != CL_SUCCESS)
        return false;


    // Create the command queue.
    mCommandQueue = clCreateCommandQueue(mContext, mDevice, 0, &err);

    if (err != CL_SUCCESS)
        return false;


    mCreated = true;
    return true;
}


void MyCLWrapper::release()
{
    clReleaseCommandQueue(mCommandQueue);
    clReleaseContext(mContext);
    clReleaseDevice(mDevice);

    mCreated = false;
}

cl_context MyCLWrapper::context() const
{
    Q_ASSERT( mCreated );
    return mContext;
}

cl_device_id MyCLWrapper::device() const
{
    Q_ASSERT( mCreated );
    return mDevice;
}

cl_command_queue MyCLWrapper::queue() const
{
    Q_ASSERT( mCreated );
    return mCommandQueue;
}
