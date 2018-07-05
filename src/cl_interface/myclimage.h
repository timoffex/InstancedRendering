#ifndef MYCLIMAGE_H
#define MYCLIMAGE_H

#include "include_opencl.h"

#include <QOpenGLTexture>

class MyCLImage2D
{
public:
    MyCLImage2D();
    ~MyCLImage2D();


    /// Creates the image with the specified width, height and format.
    bool create(cl_context context, size_t width, size_t height, cl_image_format format);

    /// Creates the image with the specified width, height and format.
    bool create(cl_context context, size_t width, size_t height, cl_channel_order channelOrder, cl_channel_type channelType = CL_FLOAT);

    /// Creates the OpenCL image to share storage with the OpenGL texture.
    bool createShared(cl_context context, const QOpenGLTexture &texture);

    /// Releases resources allocated in the create functions.
    void destroy();


    /// Returns the associated cl_image.
    const cl_image &image() const;

    /// Returns the format of the image.
    cl_image_format format() const;


    /// Returns true if this image has been acquired with acquire().
    bool isAcquired() const;

    /// Acquires the image if sharing with a GL texture. Otherwise,
    /// does nothing.
    bool acquire(cl_command_queue queue);

    /// Releases the image if sharing with a GL texture. Otherwise,
    /// does nothing.
    bool release(cl_command_queue queue);


    /// Returns true if the image has been mapped with map().
    bool isMapped() const;

    /// Maps the entire image.
    bool map(cl_command_queue queue);

    /// Maps a section of the image.
    bool map(cl_command_queue queue, size_t originX, size_t originY, size_t regionX, size_t regionY);

    /// Unmaps the image. Should be called after map().
    bool unmap(cl_command_queue queue);

    /// Sets a value in the image. The image must be mapped by calling map().
    /// x and y must be within the range defined by map().
    ///
    /// This function asserts that the format is CL_FLOAT. Extra floating point
    /// values are ignored.
    void setf(size_t x, size_t y, float v1, float v2 = 0, float v3 = 0, float v4 = 0);


    /// Determines the number of components per pixel.
    size_t numComponents() const;

    /// The size in bytes of one component (of one pixel).
    size_t componentSize() const;

    size_t width() const { return mWidth; }
    size_t height() const { return mHeight; }
private:
    bool mCreated;
    bool mFromGLTexture;
    bool mAcquired;

    /// If mFromGLTexture == true, this points to an OpenGL texture.
    const QOpenGLTexture *mOpenGLTexture;

    cl_context mContext;
    cl_image mImage;

    cl_image_format mFormat;

    size_t mWidth;
    size_t mHeight;

    bool mIsMapped;
    void *mMapPtr;
    size_t mMapOrigin[3];   /// The third component is always 0.
    size_t mMapRegion[3];   /// The third component is always 1.
};

#endif // MYCLIMAGE_H
