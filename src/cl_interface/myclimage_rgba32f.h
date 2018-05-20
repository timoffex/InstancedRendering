#ifndef MYCLIMAGE_RGBA32F_H
#define MYCLIMAGE_RGBA32F_H

#include "include_opencl.h"


#include <QOpenGLTexture>


/**
 * @brief The MyCLImage_RGBA32F class represents a cl_image with the
 * CL_FLOAT and CL_RGBA format. A more general version of such a
 * class is possible.
 */
class MyCLImage_RGBA32F
{
public:
    MyCLImage_RGBA32F();


    /// Creates the image with the specified width and height.
    bool create(cl_context context, size_t width, size_t height);

    /// Creates the image to share with the texture. The texture
    /// MUST have format CL_FLOAT / CL_RGBA.
    bool createShared(cl_context context, const QOpenGLTexture &texture);

    /// Releases the image object.
    void destroy();

    /// Returns the associated cl_image.
    cl_image image() const;


    /// Acquires the image if sharing with a GL texture. Otherwise,
    /// does nothing.
    bool acquire(cl_command_queue queue);

    /// Releases the image if sharing with a GL texture. Otherwise,
    /// does nothing.
    bool release(cl_command_queue queue);


    /// Maps the entire image.
    bool map(cl_command_queue queue);

    /// Unmaps the image. Should be called after map().
    bool unmap(cl_command_queue queue);

    /// Sets a value in the image. The image must be mapped by calling map().
    void set(size_t x, size_t y, float v1, float v2, float v3, float v4);

private:
    bool mCreated;
    bool mFromGLTexture;
    bool mAcquired;

    cl_context mContext;
    cl_image mImage;

    size_t mWidth;
    size_t mHeight;

    bool mIsMapped;
    float *mMapPtr;
};

#endif // MYCLIMAGE_RGBA32F_H
