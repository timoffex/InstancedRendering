#include "myclerrors.h"

#ifndef MYCLERRORS_ERROR_CASE
#define MYCLERRORS_ERROR_CASE(x) case x: return #x;


std::string parseCreateImageError(cl_int err)
{
    switch (err)
    {
    MYCLERRORS_ERROR_CASE(CL_INVALID_CONTEXT);
    MYCLERRORS_ERROR_CASE(CL_INVALID_VALUE);
    MYCLERRORS_ERROR_CASE(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
    MYCLERRORS_ERROR_CASE(CL_INVALID_IMAGE_DESCRIPTOR);
    MYCLERRORS_ERROR_CASE(CL_INVALID_IMAGE_SIZE);
    MYCLERRORS_ERROR_CASE(CL_INVALID_HOST_PTR);
    MYCLERRORS_ERROR_CASE(CL_IMAGE_FORMAT_NOT_SUPPORTED);
    MYCLERRORS_ERROR_CASE(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    MYCLERRORS_ERROR_CASE(CL_INVALID_OPERATION);
    MYCLERRORS_ERROR_CASE(CL_OUT_OF_RESOURCES);
    MYCLERRORS_ERROR_CASE(CL_OUT_OF_HOST_MEMORY);
    default:
        return "unknown";
    }
}

std::string parseCreateFromGLTextureError(cl_int err)
{
    switch (err)
    {
    MYCLERRORS_ERROR_CASE(CL_INVALID_CONTEXT);
    MYCLERRORS_ERROR_CASE(CL_INVALID_VALUE);
    MYCLERRORS_ERROR_CASE(CL_INVALID_MIP_LEVEL);
    MYCLERRORS_ERROR_CASE(CL_INVALID_GL_OBJECT);
    MYCLERRORS_ERROR_CASE(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
    MYCLERRORS_ERROR_CASE(CL_INVALID_OPERATION);
    MYCLERRORS_ERROR_CASE(CL_OUT_OF_RESOURCES);
    MYCLERRORS_ERROR_CASE(CL_OUT_OF_HOST_MEMORY);
    default:
        return "unknown";
    }
}

std::string parseAcquireError(cl_int err) { return "TODO"; }
std::string parseReleaseError(cl_int err) { return "TODO"; }
std::string parseMapImageError(cl_int err) { return "TODO"; }
std::string parseUnmapObjectError(cl_int err) { return "TODO"; }


#else //MYCLERRORS_ERROR_CASE

If this text causes a compilation error, the MYCLERRORS_ERROR_CASE
has been defined somewhere. That would be extremely weird.

#endif
