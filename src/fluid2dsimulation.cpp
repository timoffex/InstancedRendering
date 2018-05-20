#include "fluid2dsimulation.h"

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

    // TODO: Create and initialize a program.


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
    qFatal("Unimplemented method Fluid2DSimulation::update(float)");
    Q_UNREACHABLE();
}

bool Fluid2DSimulation::update(float dtSeconds, MyCLImage2D &forces)
{
    return mFluidProgram->updateWindNew(mVelocities.image(),
                                        forces.image(),
                                        mConfig.gridSquareSize,
                                        dtSeconds,
                                        mConfig.density,
                                        mConfig.hasViscosity ? mConfig.viscosity : -1,
                                        mPressure.image(),
                                        mTemp2F_1.image(),
                                        mTemp2F_2.image());
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
    }
    else
    {
        F2DS_CREATE_IMAGE_2F(mVelocities);
    }


    if (pressureTexture != nullptr)
    {
        if (!mPressure.createShared(wrapper->context(), *pressureTexture))
        {
            qDebug() << "Failed to instantiate mPressure from pressure texture.";
            return false;
        }
    }
    else
    {
        F2DS_CREATE_IMAGE_1F(mPressure);
    }

    F2DS_CREATE_IMAGE_2F(mTemp2F_1);
    F2DS_CREATE_IMAGE_2F(mTemp2F_2);

#undef F2DS_CREATE_IMAGE

    return true;
}
