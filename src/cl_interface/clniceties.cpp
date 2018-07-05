#include "clniceties.h"


void CLNiceties::ZeroImage(cl_command_queue queue, MyCLImage2D &zeroImage, MyCLWrapper *wrapper)
{
    if (wrapper == nullptr)
        wrapper = &MyCLWrapper::current();

    // Necessary to do anything to the image using OpenCL in the
    // case that it is shared with OpenGL.
    bool wasAcquired = zeroImage.isAcquired();
    zeroImage.acquire(queue);


    singleton().utilitiesProgram(wrapper).zeroImage(zeroImage);


    if (!wasAcquired)
        zeroImage.release(queue);
}

void CLNiceties::ZeroMappedImage(MyCLImage2D &zeroImage)
{
    Q_ASSERT(zeroImage.isMapped());

    for (size_t i = 0; i < zeroImage.width(); ++i)
        for (size_t j = 0; j < zeroImage.height(); ++j)
            zeroImage.setf(i, j, 0);
}



UtilitiesCLProgram &CLNiceties::utilitiesProgram(MyCLWrapper *wrapper)
{
    auto itr = mUtilitiesPrograms.find(wrapper);

    if (itr != mUtilitiesPrograms.end())
    {
        // The utilities program for this wrapper already exists.
        return *(itr->second);
    }
    else
    {
        // The utilities program for this wrapper doesn't yet exist,
        // so create it.
        UtilitiesCLProgram *prog = new UtilitiesCLProgram();
        Q_ASSERT( prog->create(wrapper) );

        mUtilitiesPrograms[wrapper] = prog;

        return *prog;
    }
}



CLNiceties &CLNiceties::singleton()
{
    if (static_singleton == nullptr)
        static_singleton = new CLNiceties();

    return *static_singleton;
}

CLNiceties *CLNiceties::static_singleton = nullptr;
