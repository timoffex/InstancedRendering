#ifndef MYCLWRAPPER_H
#define MYCLWRAPPER_H

#include "include_opencl.h"

#include <QOpenGLContext>


class MyCLWrapper
{
public:

    /// \brief Returns the current global context.
    ///
    /// It is asserted that the current global context is set.
    static MyCLWrapper &current();

    /// \brief Checks whether the current global context is set.
    static bool isCurrentSet();

    /// \brief Makes this the current global context.
    void makeCurrent();

    MyCLWrapper();

    /// Initializes the device, context and command queue. The
    /// context is initialized to share with the current OpenGL
    /// context.
    ///
    /// An OpenGL context must be current.
    ///
    /// Returns true on success, false on failure.
    bool createFromGLContext(cl_device_type deviceType = CL_DEVICE_TYPE_GPU);

    /// Releases the context, queue and device.
    void release();

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

    // Initialized to nullptr near the top of myclwrapper.cpp
    static MyCLWrapper *currentGlobalContext;
};

#endif // MYCLWRAPPER_H
