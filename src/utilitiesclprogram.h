#ifndef UTILITIESCLPROGRAM_H
#define UTILITIESCLPROGRAM_H

#include "cl_interface/myclwrapper.h"
#include "cl_interface/include_opencl.h"
#include "cl_interface/myclprogram.h"
#include "cl_interface/myclkernel.h"

class UtilitiesCLProgram
{
public:
    UtilitiesCLProgram();
    ~UtilitiesCLProgram();

    bool create(MyCLWrapper *wrapper);

    void destroy();

    bool zeroInitialize(MyCLImage2D &img);

private:
    bool mCreated;

    MyCLWrapper *mCLWrapper;

    MyCLProgram mProgram;
    MyCLKernel<MyCLImage2D &> mZeroInitializeKernel;
};

#endif // UTILITIESCLPROGRAM_H
