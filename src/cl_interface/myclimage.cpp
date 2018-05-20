#include "myclimage.h"

#include "myclerrors.h"

#include <QDebug>

MyCLImage2D::MyCLImage2D()
    : mCreated(false),
      mFromGLTexture(false),
      mAcquired(false),
      mOpenGLTexture(nullptr),
      mIsMapped(false),
      mMapPtr(nullptr)
{

}

MyCLImage2D::~MyCLImage2D()
{
    destroy();
}

bool MyCLImage2D::create(cl_context context, size_t width, size_t height, cl_image_format format)
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

    cl_int err;
    mImage = clCreateImage(context,
                           CL_MEM_READ_WRITE,
                           &format,
                           &desc,
                           NULL,
                           &err);

    if (err != CL_SUCCESS)
    {
        qDebug() << QString::fromStdString(parseCreateImageError(err));
        return false;
    }

    mWidth = width;
    mHeight = height;
    mFormat = format;
    mContext = context;
    mCreated = true;

    return true;
}

bool MyCLImage2D::create(cl_context context, size_t width, size_t height, cl_channel_order channelOrder, cl_channel_type channelType)
{
    cl_image_format format;
    format.image_channel_data_type = channelType;
    format.image_channel_order = channelOrder;

    return create(context, width, height, format);
}

bool MyCLImage2D::createShared(cl_context context, const QOpenGLTexture &texture)
{
    cl_int err;

    mImage = clCreateFromGLTexture(context, CL_MEM_READ_WRITE, texture.target(), 0, texture.textureId(), &err);
    if (err != CL_SUCCESS)
    {
        qDebug() << QString::fromStdString(parseCreateFromGLTextureError(err));
        return false;
    }


    err = clGetImageInfo(mImage, CL_IMAGE_FORMAT, sizeof(cl_image_format), &mFormat, NULL);
    if (err != CL_SUCCESS)
    {
        qDebug() << "Error getting image info.";
        return false;
    }

    mFromGLTexture = true;
    mOpenGLTexture = &texture;

    mContext = context;

    mWidth = texture.width();
    mHeight = texture.height();

    mCreated = true;
    return true;
}

void MyCLImage2D::destroy()
{
    if (mCreated)
    {
        clReleaseMemObject(mImage);
        mCreated = false;
    }
}

cl_image MyCLImage2D::image() const
{
    Q_ASSERT( mCreated );
    return mImage;
}

cl_image_format MyCLImage2D::format() const
{
    Q_ASSERT( mCreated );
    return mFormat;
}

bool MyCLImage2D::acquire(cl_command_queue queue)
{
    Q_ASSERT( mCreated );

    if (mFromGLTexture)
    {
        cl_int err;
        err = clEnqueueAcquireGLObjects(queue, 1, &mImage, 0, NULL, NULL);

        if (err != CL_SUCCESS)
        {
            qDebug() << QString::fromStdString(parseAcquireError(err));
            return false;
        }

        mAcquired = true;
    }

    return true;
}

bool MyCLImage2D::release(cl_command_queue queue)
{
    Q_ASSERT( mCreated );

    if (mFromGLTexture)
    {
        Q_ASSERT( mAcquired );

        cl_int err;
        err = clEnqueueReleaseGLObjects(queue, 1, &mImage, 0, NULL, NULL);;

        if (err != CL_SUCCESS)
        {
            qDebug() << QString::fromStdString(parseAcquireError(err));
            return false;
        }

        mAcquired = false;
    }

    return true;
}

bool MyCLImage2D::map(cl_command_queue queue)
{
    return map(queue, 0, 0, mWidth, mHeight);
}

bool MyCLImage2D::map(cl_command_queue queue, size_t originX, size_t originY, size_t regionX, size_t regionY)
{
    Q_ASSERT( mCreated );

    mMapOrigin[0] = originX;
    mMapOrigin[1] = originY;
    mMapOrigin[2] = 0;

    mMapRegion[0] = regionX;
    mMapRegion[1] = regionY;
    mMapRegion[2] = 1;

    size_t rowPitch;

    cl_int err;

    mMapPtr = clEnqueueMapImage(queue,
                                mImage,
                                CL_TRUE,
                                CL_MAP_READ | CL_MAP_WRITE,
                                mMapOrigin, mMapRegion,
                                &rowPitch, NULL,
                                0, NULL, NULL,
                                &err);

    Q_ASSERT( rowPitch == mWidth * numComponents() * componentSize() );

    if (err != CL_SUCCESS)
    {
        qDebug() << QString::fromStdString(parseMapImageError(err));
        return false;
    }

    mIsMapped = true;
    return true;
}

bool MyCLImage2D::unmap(cl_command_queue queue)
{
    Q_ASSERT( mIsMapped );

    cl_int err = clEnqueueUnmapMemObject(queue, mImage, mMapPtr, 0, NULL, NULL);

    if (err != CL_SUCCESS)
    {
        qDebug() << QString::fromStdString(parseUnmapObjectError(err));
        return false;
    }

    mIsMapped = false;
    mMapPtr = nullptr;
    return true;
}

void MyCLImage2D::setf(size_t x, size_t y, float v1, float v2, float v3, float v4)
{
    Q_ASSERT( mIsMapped );
    Q_ASSERT( mFormat.image_channel_data_type == CL_FLOAT );

    Q_ASSERT( componentSize() == sizeof(float) );

    // Pixel size in floats.
    size_t pixelSize = numComponents();

    float *ptr = (float *)mMapPtr;

    switch (numComponents())
    {
    case 4:
        ptr[y * mWidth * pixelSize + x * pixelSize + 3] = v4;
    case 3:
        ptr[y * mWidth * pixelSize + x * pixelSize + 2] = v3;
    case 2:
        ptr[y * mWidth * pixelSize + x * pixelSize + 1] = v2;
    case 1:
        ptr[y * mWidth * pixelSize + x * pixelSize + 0] = v1;
        break;
    default:
        qFatal("Unhandled component number in setf()");
    }
}

size_t MyCLImage2D::numComponents() const
{
    Q_ASSERT( mCreated );

    switch (mFormat.image_channel_order)
    {
    case CL_R:
    case CL_A:
        return 1;
    case CL_RG:
    case CL_RA:
        return 2;
    case CL_RGB:
        return 3;
    case CL_RGBA:
    case CL_BGRA:
    case CL_ARGB:
        return 4;

    case CL_INTENSITY:
    case CL_LUMINANCE:
        qFatal("I don't know the number of components in a CL_INTENSITY or a CL_LUMINANCE image.");

    default:
        qFatal("Unknown image format channel order.");
    }

    Q_UNREACHABLE();
}

size_t MyCLImage2D::componentSize() const
{
    Q_ASSERT( mCreated );

    switch (mFormat.image_channel_data_type)
    {
    case CL_SNORM_INT8:
    case CL_UNORM_INT8:
    case CL_SIGNED_INT8:
    case CL_UNSIGNED_INT8:
        return 1;
    case CL_SNORM_INT16:
    case CL_UNORM_INT16:
    case CL_SIGNED_INT16:
    case CL_UNSIGNED_INT16:
    case CL_HALF_FLOAT:
        return 2;
    case CL_SIGNED_INT32:
    case CL_UNSIGNED_INT32:
    case CL_FLOAT:
        return 4;


    case CL_UNORM_SHORT_565:
        qFatal("CL_UNORM_SHORT_565 has uneven component sizes.");
    case CL_UNORM_SHORT_555:
        qFatal("CL_UNORM_SHORT_555 components aren't byte-sized.");
    case CL_UNORM_INT_101010:
        qFatal("CL_UNORM_INT_101010 components aren't byte-sized.");

    default:
        qFatal("Unknown image format data type.");
    }
}
