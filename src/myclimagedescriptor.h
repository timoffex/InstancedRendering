#ifndef MYCLIMAGEDESCRIPTOR_H
#define MYCLIMAGEDESCRIPTOR_H

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#endif

class MyCLImageDescriptor
{
public:
    static cl_image_desc from(cl_image img);
};

#endif // MYCLIMAGEDESCRIPTOR_H
