#include "fluid2dsimulation.h"
#include "cl_interface/clniceties.h"

Fluid2DSimulation::Fluid2DSimulation(Fluid2DSimulationConfig config)
    : mInitialized(false),
      mConfig(config)
{
}

Fluid2DSimulation::~Fluid2DSimulation()
{
    release(); // release() checks mInitialized
}

bool Fluid2DSimulation::create(MyCLWrapper *wrapper,
                               const QOpenGLTexture *velocityTexture,
                               const QOpenGLTexture *pressureTexture)
{

    if (!mFluidProgram.create(wrapper))
        return false;

    if (!createImages(wrapper, velocityTexture, pressureTexture))
        return false;


    mCLWrapper = wrapper;
    mInitialized = true;
    return true;
}

void Fluid2DSimulation::release()
{
    if (mInitialized)
    {
        mVelocities.destroy();
        mPressure.destroy();
        mTemp2F_2.destroy();
        mTemp2F_1.destroy();
        mInitialized = false;
    }
}

bool Fluid2DSimulation::update(float dtSeconds)
{
    if (!mVelocities.acquire(mCLWrapper->queue())) return false;
    if (!mPressure.acquire(mCLWrapper->queue())) return false;

    if (!mFluidProgram.update(mVelocities,
                              NULL,
                              mPressure,
                              mTemp2F_1,
                              mTemp2F_2,
                              mConfig.gridSquareSize,
                              dtSeconds,
                              mConfig.density,
                              mConfig.hasViscosity ? mConfig.viscosity : -1))
    {
        qDebug() << "Failed in wind update.";
        return false;
    }

    if (!mPressure.release(mCLWrapper->queue())) return false;
    if (!mVelocities.release(mCLWrapper->queue())) return false;

    return true;
}

bool Fluid2DSimulation::update(float dtSeconds, MyCLImage2D &forces)
{
    if (!mVelocities.acquire(mCLWrapper->queue())) return false;
    if (!mPressure.acquire(mCLWrapper->queue())) return false;

    if (!mFluidProgram.update(mVelocities,
                              &forces,
                              mPressure,
                              mTemp2F_1,
                              mTemp2F_2,
                              mConfig.gridSquareSize,
                              dtSeconds,
                              mConfig.density,
                              mConfig.hasViscosity ? mConfig.viscosity : -1))
    {
        qDebug() << "Failed in wind update.";
        return false;
    }

    if (!mPressure.release(mCLWrapper->queue())) return false;
    if (!mVelocities.release(mCLWrapper->queue())) return false;

    return true;
}


bool Fluid2DSimulation::createImages(MyCLWrapper *wrapper,
                                     const QOpenGLTexture *velocityTexture,
                                     const QOpenGLTexture *pressureTexture)
{
// The macro should not have been defined anywhere else.
#ifdef F2DS_CREATE_IMAGE
    static_assert(false);
#endif

// This macro is defined to avoid some error-handling repetition.
#define F2DS_CREATE_IMAGE(var, order)\
    if (!var.create(wrapper->context(),\
                    mConfig.width,\
                    mConfig.height,\
                    order, CL_FLOAT))\
    {\
        qDebug() << "Failed to instantiate " #var;\
        return false;\
    }

#define F2DS_CREATE_IMAGE_2F(var) F2DS_CREATE_IMAGE(var, CL_RG)
#define F2DS_CREATE_IMAGE_1F(var) F2DS_CREATE_IMAGE(var, CL_R)

    if (velocityTexture != nullptr)
    {
        if (!mVelocities.createShared(wrapper->context(), *velocityTexture))
        {
            qDebug() << "Failed to instantiate mVelocities from velocity texture.";
            return false;
        }

        if (mConfig.zeroInitializeSharedTextures)
            CLNiceties::ZeroImage(wrapper->queue(), mVelocities);
    }
    else
    {
        F2DS_CREATE_IMAGE_2F(mVelocities);
        CLNiceties::ZeroImage(wrapper->queue(), mVelocities);
    }


    if (pressureTexture != nullptr)
    {
        if (!mPressure.createShared(wrapper->context(), *pressureTexture))
        {
            qDebug() << "Failed to instantiate mPressure from pressure texture.";
            return false;
        }

        if (mConfig.zeroInitializeSharedTextures)
            CLNiceties::ZeroImage(wrapper->queue(), mPressure);
    }
    else
    {
        F2DS_CREATE_IMAGE_1F(mPressure);
        CLNiceties::ZeroImage(wrapper->queue(), mPressure);
    }





    F2DS_CREATE_IMAGE_2F(mTemp2F_1);
    F2DS_CREATE_IMAGE_2F(mTemp2F_2);

    CLNiceties::ZeroImage(wrapper->queue(), mTemp2F_1);
    CLNiceties::ZeroImage(wrapper->queue(), mTemp2F_2);

#undef F2DS_CREATE_IMAGE

    return true;
}
