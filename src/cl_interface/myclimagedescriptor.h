#ifndef MYCLIMAGEDESCRIPTOR_H
#define MYCLIMAGEDESCRIPTOR_H

#include "include_opencl.h"

class MyCLImageDescriptor
{
public:
    static cl_image_desc from(cl_image img);
};

#endif // MYCLIMAGEDESCRIPTOR_H
