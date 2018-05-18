#ifndef MYCLIMAGE_RGBA32F_H
#define MYCLIMAGE_RGBA32F_H

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#endif


#include <QOpenGLTexture>


class MyCLImage_RGBA32F
{
public:
    MyCLImage_RGBA32F();

    /// Creates the image with the specified description.
    bool create(cl_context context, cl_image_desc desc);

    /// Creates the image with the specified width and height.
    bool create(cl_context context, size_t width, size_t height);

    /// Creates the image to share with the texture.
    bool create(cl_context context, const QOpenGLTexture &texture);

    cl_image image() const;


    bool acquire(cl_command_queue queue);
    bool release(cl_command_queue queue);


    bool map(cl_command_queue queue);
    bool unmap(cl_command_queue queue);

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
