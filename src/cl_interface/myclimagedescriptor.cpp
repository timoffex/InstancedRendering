#include "myclimagedescriptor.h"

#include <QDebug>


cl_image_desc MyCLImageDescriptor::from(cl_image img)
{
    cl_image_desc desc;
    desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    desc.image_array_size = 1;
    desc.image_row_pitch = 0;
    desc.image_slice_pitch = 0;
    desc.num_mip_levels = 0;
    desc.num_samples = 0;
    desc.buffer = NULL;

    cl_int err;
    err = clGetImageInfo(img, CL_IMAGE_WIDTH, sizeof(size_t), &desc.image_width, NULL);
    err = clGetImageInfo(img, CL_IMAGE_HEIGHT, sizeof(size_t), &desc.image_height, NULL);
    err = clGetImageInfo(img, CL_IMAGE_DEPTH, sizeof(size_t), &desc.image_depth, NULL);

    if (err != CL_SUCCESS)
        qFatal("Failed to retrieve some image descriptor information.");

    return desc;
}
