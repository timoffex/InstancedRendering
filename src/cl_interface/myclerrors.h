#ifndef MYCLERRORS_H
#define MYCLERRORS_H

#include <string>

#include "include_opencl.h"


std::string parseCreateImageError(cl_int err);
std::string parseCreateFromGLTextureError(cl_int err);
std::string parseAcquireError(cl_int err);
std::string parseReleaseError(cl_int err);
std::string parseMapImageError(cl_int err);
std::string parseUnmapObjectError(cl_int err);

std::string parseBuildReturnCode(cl_int err);
std::string parseEnqueueKernelReturnCode(cl_int err);


#endif // MYCLERRORS_H
