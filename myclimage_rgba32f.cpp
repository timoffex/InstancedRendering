#include "myclimage_rgba32f.h"

#include <QDebug>

MyCLImage_RGBA32F::MyCLImage_RGBA32F()
    : mCreated(false),
      mFromGLTexture(false),
      mAcquired(true),
      mIsMapped(false),
      mMapPtr(nullptr)
{

}


bool MyCLImage_RGBA32F::create(cl_context context, cl_image_desc desc)
{
    // TODO: Need to assert some things about desc.

    cl_image_format format;
    format.image_channel_data_type = CL_FLOAT;
    format.image_channel_order = CL_RGBA;

    cl_int err;
    mImage = clCreateImage(context,
                           CL_MEM_READ_WRITE,
                           &format,
                           &desc,
                           NULL,
                           &err);

    if (err != CL_SUCCESS)
        return false;

    mContext = context;
    mWidth = desc.image_width;
    mHeight = desc.image_height;
    mCreated = true;
    return true;
}

bool MyCLImage_RGBA32F::create(cl_context context, size_t width, size_t height)
{
    cl_image_desc desc;
    desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    desc.image_array_size = 1;
    desc.image_row_pitch = 0;
    desc.image_slice_pitch = 0;
    desc.num_mip_levels = 0;
    desc.num_samples = 0;
    desc.buffer = NULL;

    desc.image_width = width;
    desc.image_height = height;
    desc.image_depth = 1;

    return create(context, desc);
}


bool MyCLImage_RGBA32F::create(cl_context context, const QOpenGLTexture &texture)
{
    // TODO: Assert facts about texture (e.g. there needs to be a specific format).

    cl_int err;

    mImage = clCreateFromGLTexture(context, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, texture.textureId(), &err);

    if (err != CL_SUCCESS)
        return false;

    mContext = context;
    mWidth = texture.width();
    mHeight = texture.height();
    mCreated = true;
    mFromGLTexture = true;
    mAcquired = false;
    return true;
}


cl_image MyCLImage_RGBA32F::image() const
{
    Q_ASSERT( mCreated );
    return mImage;
}


bool MyCLImage_RGBA32F::acquire(cl_command_queue queue)
{
    if (mFromGLTexture) {
        cl_int err = clEnqueueAcquireGLObjects(queue, 1, &mImage, 0, NULL, NULL);

        if (err != CL_SUCCESS)
            return false;

        mAcquired = true;
    }

    return true;
}

bool MyCLImage_RGBA32F::release(cl_command_queue queue)
{
    if (mFromGLTexture) {
        cl_int err = clEnqueueReleaseGLObjects(queue, 1, &mImage, 0, NULL, NULL);

        if (err != CL_SUCCESS)
            return false;

        mAcquired = false;
    }

    return true;
}

bool MyCLImage_RGBA32F::map(cl_command_queue queue)
{
    Q_ASSERT( mCreated );
    Q_ASSERT( mAcquired );
    cl_int err;

    size_t origin[3] = {0,0,0};
    size_t region[3] = {mWidth, mHeight, 1};

    size_t rowPitch;

    mMapPtr = (float *)clEnqueueMapImage(queue,
                                         mImage,
                                         CL_TRUE,
                                         CL_MAP_READ | CL_MAP_WRITE,
                                         origin, region,
                                         &rowPitch, NULL,
                                         0, NULL, NULL,
                                         &err);

    // My code only handles this case for now.
    Q_ASSERT( rowPitch == mWidth * 4 * sizeof(float) );

    if (err != CL_SUCCESS)
        return false;

    mIsMapped = true;
    return true;
}

bool MyCLImage_RGBA32F::unmap(cl_command_queue queue)
{
    Q_ASSERT( mIsMapped );
    cl_int err = clEnqueueUnmapMemObject(queue, mImage, mMapPtr, 0, NULL, NULL);

    if (err != CL_SUCCESS)
        return false;

    mIsMapped = false;
    mMapPtr = nullptr;
    return true;
}

void MyCLImage_RGBA32F::set(size_t x, size_t y, float v1, float v2, float v3, float v4)
{
    Q_ASSERT( mIsMapped );

    Q_ASSERT( x >= 0 && x < mWidth );
    Q_ASSERT( y >= 0 && y < mHeight );

    mMapPtr[y * mWidth * 4 + x * 4 + 0] = v1;
    mMapPtr[y * mWidth * 4 + x * 4 + 1] = v2;
    mMapPtr[y * mWidth * 4 + x * 4 + 2] = v3;
    mMapPtr[y * mWidth * 4 + x * 4 + 3] = v4;
}
