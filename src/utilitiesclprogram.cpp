#include "utilitiesclprogram.h"

#include "cl_interface/myclerrors.h"

UtilitiesCLProgram::UtilitiesCLProgram()
    : mCreated(false)
{

}


UtilitiesCLProgram::~UtilitiesCLProgram()
{
    destroy();
}


bool UtilitiesCLProgram::create(MyCLWrapper *wrapper)
{
    mCLWrapper = wrapper;

    if (!mProgram.create(wrapper, ":/compute/utilities.cl"))
    {
        qDebug() << "Couldn't create utilities program.";
        return false;
    }

    if (!mZeroInitializeKernel.createFromProgram(wrapper, mProgram.program(), "zeroInitialize"))
    {
        qDebug() << "Couldn't create zeroInitialize kernel.";
        return false;
    }

    mCreated = true;
    return true;
}


void UtilitiesCLProgram::destroy()
{
    if (mCreated)
    {
        mZeroInitializeKernel.destroy();
        mProgram.destroy();
        mCreated = false;
    }
}


bool UtilitiesCLProgram::zeroImage(MyCLImage2D &img)
{
    return mZeroInitializeKernel(img.width(), img.height(), img.image(), img.width(), img.height());
}
