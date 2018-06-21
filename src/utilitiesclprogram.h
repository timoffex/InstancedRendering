#ifndef UTILITIESCLPROGRAM_H
#define UTILITIESCLPROGRAM_H

#include "cl_interface/myclwrapper.h"
#include "cl_interface/include_opencl.h"

class UtilitiesCLProgram
{
public:
    UtilitiesCLProgram();

    bool create(MyCLWrapper *wrapper);

    void release();

    bool zeroInitialize(cl_mem img);

private:
    bool mCreated;

    MyCLWrapper *mCLWrapper;

    cl_program mProgram;
    cl_kernel mZeroInitializeKernel;
};

#endif // UTILITIESCLPROGRAM_H
