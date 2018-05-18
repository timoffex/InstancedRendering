#include "imagetestwindow.h"

#include <QtMath>

ImageTestWindow::ImageTestWindow()
{

}


void ImageTestWindow::initializeGL()
{
    initializeOpenGLFunctions();

    createScreenQuadProgram();
    createScreenQuadData();
    createScreenQuadTexture();

    initializeCL();
}


void ImageTestWindow::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void ImageTestWindow::paintGL()
{
    doUpdate();
    if (clFinish(mCLWrapper.queue()) != CL_SUCCESS)
        qFatal("clFinish() in paintGL() failed.");

    glClear(GL_COLOR_BUFFER_BIT);

    mScreenQuadProgram->bind();

    mScreenQuadProgram->setWindSpeedTextureUnit(0);

    glActiveTexture(GL_TEXTURE0);
//    mWindTexture->bind();
    mPressureTexture->bind();

    mScreenQuadVAO->bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    mScreenQuadVAO->release();

    mScreenQuadProgram->release();

    if (checkGLErrors())
        qDebug() << "^ in paintGL()";

    update();
}

void ImageTestWindow::doUpdate()
{
    mWindSpeeds.acquire(mCLWrapper.queue());
    mForces.acquire(mCLWrapper.queue());
    mPressure.acquire(mCLWrapper.queue());
    mTemp1.acquire(mCLWrapper.queue());
    mTemp2.acquire(mCLWrapper.queue());

    float h = 0.4;
    float dt = 0.1;
    float visc = 0.0002;
    float density = 1.5;

    // TEST: Advect wind speed by itself. SUCCESS
//    mWindCLProgram.advect(mWindSpeeds.image(), mWindSpeeds.image(), mTemp1.image(), dt, h);
//    mWindCLProgram.advect(mTemp1.image(), mTemp1.image(), mWindSpeeds.image(), dt, h);

    // TEST: Diffuse wind speed. SUCCESS
//    float hh_dt = h*h / dt;
//    mWindCLProgram.jacobi(mImage1.image(), mImage1.image(), mImage2.image(), hh_dt, 4 + hh_dt);
//    mWindCLProgram.jacobi(mImage2.image(), mImage2.image(), mImage1.image(), hh_dt, 4 + hh_dt);

    // TEST: Gradient computation. SUCCESS
//    static int times = 0;
//    if (times == 10)
//    {
//        mWindCLProgram.gradient(mWindSpeeds.image(), mTemp1.image(), h);
//        mWindCLProgram.addScaled(mTemp1.image(), mTemp2.image(), 0, mWindSpeeds.image());
//    }
//    ++times;

    // TEST: Full wind procedure.
    mWindCLProgram.updateWindNew(mWindSpeeds.image(), mForces.image(), h, dt, density, visc, mPressure.image(), mTemp1.image(), mTemp2.image());


    mTemp2.release(mCLWrapper.queue());
    mTemp1.release(mCLWrapper.queue());
    mPressure.release(mCLWrapper.queue());
    mForces.release(mCLWrapper.queue());
    mWindSpeeds.release(mCLWrapper.queue());
}

static void ERROR_IF_FALSE(bool cond, const char *msg)
{
    if (!cond)
        qFatal("%s", msg);
}


void ImageTestWindow::createScreenQuadProgram()
{
    mScreenQuadProgram = new WindQuadGLProgram();

    if (!mScreenQuadProgram->create())
        qFatal("Failed to create wind quad program.");

    if (checkGLErrors())
        qDebug() << "^ in createScreenQuadProgram().";
}

void ImageTestWindow::createScreenQuadData()
{
    float screenQuad[] = {
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f,

        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f,
        -1.0f,  1.0f,   0.0f, 1.0f
    };

    mScreenQuadBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    ERROR_IF_FALSE(mScreenQuadBuffer->create(), "Couldn't create mScreenQuadBuffer.");
    ERROR_IF_FALSE(mScreenQuadBuffer->bind(), "Couldn't bind mScreenQuadBuffer.");
    mScreenQuadBuffer->allocate(screenQuad, sizeof(screenQuad));
    mScreenQuadBuffer->release();

    mScreenQuadVAO = new QOpenGLVertexArrayObject(this);
    ERROR_IF_FALSE(mScreenQuadVAO->create(), "Couldn't create mScreenQuadVAO.");

    mScreenQuadVAO->bind();
    mScreenQuadProgram->enableAllAttributeArrays();

    mScreenQuadBuffer->bind();
    mScreenQuadProgram->setPositionBuffer(0, 4 * sizeof(float));
    mScreenQuadProgram->setTexCoordBuffer(2 * sizeof(float), 4 * sizeof(float));
    mScreenQuadBuffer->release();
    mScreenQuadVAO->release();

    if (checkGLErrors())
        qDebug() << "^ in createScreenQuadData().";
}

void ImageTestWindow::createScreenQuadTexture()
{
    mWindTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);

    ERROR_IF_FALSE(mWindTexture->create(), "Couldn't create screen quad texture.");
    mWindTexture->setFormat(QOpenGLTexture::RGBA32F);
    mWindTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
    mWindTexture->setMinificationFilter(QOpenGLTexture::Nearest);
    mWindTexture->setAutoMipMapGenerationEnabled(false);
    mWindTexture->setSize(128, 128);
    mWindTexture->allocateStorage();


    mPressureTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);

    ERROR_IF_FALSE(mPressureTexture->create(), "Couldn't create screen quad texture.");
    mPressureTexture->setFormat(QOpenGLTexture::RGBA32F);
    mPressureTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
    mPressureTexture->setMinificationFilter(QOpenGLTexture::Nearest);
    mPressureTexture->setAutoMipMapGenerationEnabled(false);
    mPressureTexture->setSize(128, 128);
    mPressureTexture->allocateStorage();
}


void ImageTestWindow::initializeCL()
{
    ERROR_IF_FALSE(mCLWrapper.create(), "Failed to create CL context.");
    ERROR_IF_FALSE(mWindCLProgram.create(&mCLWrapper), "Failed to create WindCLProgram.");

    ERROR_IF_FALSE(mWindSpeeds.create(mCLWrapper.context(), *mWindTexture), "Failed to create mImage1.");
    ERROR_IF_FALSE(mForces.create(mCLWrapper.context(), mWindTexture->width(), mWindTexture->height()), "Failed to create mImage2.");
    ERROR_IF_FALSE(mPressure.create(mCLWrapper.context(), *mPressureTexture), "Failed to create mImage3.");
    ERROR_IF_FALSE(mTemp1.create(mCLWrapper.context(), mWindTexture->width(), mWindTexture->height()), "Failed to create mImage4.");
    ERROR_IF_FALSE(mTemp2.create(mCLWrapper.context(), mWindTexture->width(), mWindTexture->height()), "Failed to create mImage5.");



    mWindSpeeds.acquire(mCLWrapper.queue());
    mWindSpeeds.map(mCLWrapper.queue());

    for (int x = 50; x < 70; ++x) {
        for (int y = 50; y < 70; ++y) {
//            float dx = (x - 60)/5;
//            float dy = (y - 60)/5;

//            mWindSpeeds.set(x, y, 0.4 * exp(-dx*dx - dy*dy), 0, 0, 0);
            mWindSpeeds.set(x, y, 0.4, 0, 0, 0);
        }
    }

    mWindSpeeds.unmap(mCLWrapper.queue());
    mWindSpeeds.release(mCLWrapper.queue());

    mForces.acquire(mCLWrapper.queue());
    mForces.map(mCLWrapper.queue());

    for (int x = 50; x < 70; ++x)
        mForces.set(x, 1, 1, 1, 0, 0);

    mForces.unmap(mCLWrapper.queue());
    mForces.release(mCLWrapper.queue());

    if (clFinish(mCLWrapper.queue()) != CL_SUCCESS)
        qFatal("clFinish() failed in initializeGL()");
}


bool ImageTestWindow::checkGLErrors()
{
    bool hadError = false;

    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR)
    {
        switch (error) {
        case GL_INVALID_OPERATION:
            qDebug() << "Invalid operation.";
            break;
        case GL_INVALID_VALUE:
            qDebug() << "Invalid value.";
            break;
        default:
            qDebug() << "Unprocessed error.";
            break;
        }

        hadError = true;
    }

    return hadError;
}
