#ifndef MYCLWRAPPER_H
#define MYCLWRAPPER_H


// This include usually only works on Macs. For other systems,
// the correct include is probably <cl.h>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#endif


#include <QOpenGLContext>

class MyCLWrapper
{
public:
    MyCLWrapper();

    /// Initializes the device, context and command queue. The
    /// context is initialized to share with the current OpenGL
    /// context.
    ///
    /// An OpenGL context must be current.
    ///
    /// Returns true on success, false on failure.
    bool create(cl_device_type deviceType = CL_DEVICE_TYPE_GPU);

    /// Returns the context. create() must have been called.
    cl_context context() const;

    /// Returns the device. create() must have been called.
    cl_device_id device() const;

    /// Returns the command queue. create() must have been called.
    cl_command_queue queue() const;

private:

    bool mCreated;

    cl_device_id mDevice;
    cl_context mContext;
    cl_command_queue mCommandQueue;

};

#endif // MYCLWRAPPER_H