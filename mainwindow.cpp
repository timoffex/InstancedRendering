#include "mainwindow.h"

#include "grass.h"

// For rand()
#include <cstdlib>

#include <QtMath>

#include <fstream>

MainWindow::MainWindow(QWindow *parent)
    : QOpenGLWindow(NoPartialUpdate, parent)
{
    mApplicationStartTime = QTime::currentTime();
}

MainWindow::~MainWindow()
{
    // shader programs and VAOs have this object as the parent

    delete mGrassBladeModelBuffer;
    delete mGrassBladeInstancedBuffer;

    // TODO: This, and probably the above deletes, must happen with a current context.
//    delete mGrassTexture;
}


static void ERROR(const char *msg)
{
    qDebug() << "ERROR: " << msg;
    exit(1);
}

static void ERROR_IF_FALSE(bool cond, const char *msg)
{
    if (!cond)
        ERROR(msg);
}

static void ERROR_IF_NOT_SUCCESS(cl_int err, const char *msg)
{
    if (err != CL_SUCCESS)
        ERROR(msg);
}

void MainWindow::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);

    initializeCamera();

    // Create the OpenGL shader program for rendering grass.
    ERROR_IF_FALSE(mGrassProgram.create(), "Failed to create grass program.");


    // Initialize OpenCL.
    mCLWrapper = new MyCLWrapper();
    ERROR_IF_FALSE(mCLWrapper->create(), "Failed to initialize OpenCL.");

    // Create the OpenCL program for wind effects.
    mWindProgram = new GrassWindCLProgram();
    ERROR_IF_FALSE(mWindProgram->create(mCLWrapper), "Failed to create wind program.");


    createGrassModel(45);
    createGrassInstanceData();
    createGrassVAO();

    if (checkGLErrors())
        qDebug() << "^ in initializeGL()";


    createCLBuffersFromGLBuffers();


    // TODO
    mWindVelocities = new QOpenGLTexture(QOpenGLTexture::Target2D);
    ERROR_IF_FALSE(mWindVelocities->create(), "Couldn't create wind texture.");
    mWindVelocities->setFormat(QOpenGLTexture::RGBA32F);
    mWindVelocities->setMagnificationFilter(QOpenGLTexture::Nearest);
    mWindVelocities->setMinificationFilter(QOpenGLTexture::Nearest);
    mWindVelocities->setAutoMipMapGenerationEnabled(false);
    mWindVelocities->setSize(128, 128);
    mWindVelocities->allocateStorage();


    ERROR_IF_FALSE(mWindVelocities1.create(mCLWrapper->context(), *mWindVelocities), "Failed to create a CL image.");
    ERROR_IF_FALSE(mForces1.create(mCLWrapper->context(), mWindVelocities->width(), mWindVelocities->height()), "Failed to create a CL image.");
    ERROR_IF_FALSE(mForces2.create(mCLWrapper->context(), mWindVelocities->width(), mWindVelocities->height()), "Failed to create a CL image.");
    ERROR_IF_FALSE(mPressure.create(mCLWrapper->context(), mWindVelocities->width(), mWindVelocities->height()), "Failed to create a CL image.");
    ERROR_IF_FALSE(mTemp1.create(mCLWrapper->context(), mWindVelocities->width(), mWindVelocities->height()), "Failed to create a CL image.");
    ERROR_IF_FALSE(mTemp2.create(mCLWrapper->context(), mWindVelocities->width(), mWindVelocities->height()), "Failed to create a CL image.");

    // Initialize forces.
    mForces1.acquire(mCLWrapper->queue());
    mForces1.map(mCLWrapper->queue());
    for (int x = 50; x < 70; ++x)
        mForces1.set(x, 1, 5, 5, 0, 0);
    mForces1.unmap(mCLWrapper->queue());
    mForces1.release(mCLWrapper->queue());

//    mForces2.acquire(mCLWrapper->queue());
//    mForces2.map(mCLWrapper->queue());
//    for (int y = 20; y < 30; ++y)
//        mForces2.set(30, y, -10, 0, 0, 0);
//    mForces2.unmap(mCLWrapper->queue());
//    mForces2.release(mCLWrapper->queue());


    mCurForce = &mForces1;
    mNextForce = &mForces2;


    ERROR_IF_FALSE(mWindQuadProgram.create(), "Failed to create wind quad program.");

    float windQuad[] = {
        0.5f, 0.5f,   1.0f, 0.0f,
        1.0f, 0.5f,   0.0f, 0.0f,
        1.0f, 1.0f,   0.0f, 1.0f,

        0.5f, 0.5f,   1.0f, 0.0f,
        1.0f, 1.0f,   0.0f, 1.0f,
        0.5f, 1.0f,   1.0f, 1.0f
    };

    mWindQuadBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    ERROR_IF_FALSE(mWindQuadBuffer->create(), "Couldn't create mWindQuadBuffer.");
    ERROR_IF_FALSE(mWindQuadBuffer->bind(), "Couldn't bind mWindQuadBuffer.");
    mWindQuadBuffer->allocate(windQuad, sizeof(windQuad));
    mWindQuadBuffer->release();

    mWindQuadVAO = new QOpenGLVertexArrayObject(this);
    ERROR_IF_FALSE(mWindQuadVAO->create(), "Couldn't create mWindQuadVAO.");

    mWindQuadVAO->bind();
    mWindQuadProgram.enableAllAttributeArrays();

    mWindQuadBuffer->bind();
    mWindQuadProgram.setPositionBuffer(0, 4 * sizeof(float));
    mWindQuadProgram.setTexCoordBuffer(2 * sizeof(float), 4 * sizeof(float));
    mWindQuadBuffer->release();
    mWindQuadVAO->release();


    if (checkGLErrors())
        qDebug() << "^ in initializeGL() at end";
}

void MainWindow::resizeGL(int w, int h)
{
    // TODO Do nothing for now. Later, use this to change up projection matrix when aspect changes.
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void MainWindow::paintGL()
{
    updateWind();
    updateGrassWindOffsets();

    ERROR_IF_NOT_SUCCESS(clFinish(mCLWrapper->queue()), "Failed to finish OpenCL commands.");

    glViewport(0, 0, width(), height());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 perspectiveMat;
    perspectiveMat.perspective(90, (float) width() / height(), 0.1, 250);
    QMatrix4x4 viewMat = getViewMatrix();

    ERROR_IF_FALSE(mGrassProgram.bind(), "Failed to bind program in paint call.");

    mGrassProgram.setMVP(perspectiveMat * viewMat);
    mGrassProgram.setGrassTextureUnit(0);

    glActiveTexture(GL_TEXTURE0);
    mGrassTexture->bind(); // binds to texture unit 0

    mGrassVAO->bind();
    glDrawArraysInstanced(GL_TRIANGLES, 0, mNumVerticesPerBlade, mNumBlades);
    mGrassVAO->release();

    mGrassTexture->release();

    mGrassProgram.release();

    /* TODO:
        I need to create a special program just for drawing a textured quad.
        I need to actually draw a quad, which MAY involve creating a VAO. SMH!
            -- Steps:
                1) Make buffer.
                2) Make VAO.
                3) Enable vertex arrays.
                4) Bind buffer.
                5) Set vertex attribute buffer.
        After that, I should clean up the wind texture initialization and
        all the wind program code (the CL side and the host side). */


    if (checkGLErrors())
        qDebug() << "^ in paintGL() after drawing grass";

    ERROR_IF_FALSE(mWindQuadProgram.bind(), "Couldn't bind mWindQuadProgram.");

    mWindQuadProgram.setWindSpeedTextureUnit(0);
    glActiveTexture(GL_TEXTURE0);
    mWindVelocities->bind();

    mWindQuadVAO->bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    mWindQuadVAO->release();

    mWindQuadProgram.release();

    if (checkGLErrors())
        qDebug() << "^ in paintGL() at end";

    // Schedule another update.
    update();
}


void MainWindow::mouseMoveEvent(QMouseEvent *evt)
{
    if (mDragging)
    {
        if (evt->modifiers() == Qt::ShiftModifier)
        {
            // Change offset if the shift key is pressed.
            QVector2D delta(evt->pos() - mDragStart);

            QVector4D offset(-delta.x(), delta.y(), 0, 1);

            QMatrix4x4 rotation;
            rotation.rotate(-mCameraYaw, 0, 1);
            rotation.rotate(-mCameraPitch, 1, 0);


            offset = rotation * offset * mCameraMoveSensitivity;
            offset.setY(0);

            mCameraOffset += offset.toVector3D();
        }
        else
        {
            // Change angles if the shift key is not pressed.
            mCameraPitch = mDragPitchStart + (evt->y() - mDragStart.y()) / 5.0;
            mCameraYaw = mDragYawStart + (evt->x() - mDragStart.x()) / 5.0;
        }

        beginDrag(evt);

        update();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *evt)
{
    if (evt->button() == Qt::LeftButton)
    {
        Q_ASSERT( !mDragging );
        beginDrag(evt);
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *evt)
{
    if (evt->button() == Qt::LeftButton)
    {
        mDragging = false;
    }
}

void MainWindow::wheelEvent(QWheelEvent *evt)
{
    mCameraZoom *= pow(1.1, evt->angleDelta().y() * mCameraZoomSensitivity);
}

void MainWindow::keyReleaseEvent(QKeyEvent *evt)
{
    if (evt->key() == Qt::Key_F)
    {
        // Toggle force.
        MyCLImage_RGBA32F *tmp = mCurForce;
        mCurForce = mNextForce;
        mNextForce = tmp;
    }
}

bool MainWindow::checkGLErrors()
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
            qDebug() << "Unprocessed GL error.";
            break;
        }

        hadError = true;
    }

    return hadError;
}

void MainWindow::beginDrag(QMouseEvent *evt)
{
    mDragging = true;
    mDragStart = evt->pos();
    mDragPitchStart = mCameraPitch;
    mDragYawStart = mCameraYaw;
}

void MainWindow::initializeCamera()
{
    mCameraOffset = QVector3D(15, 0, -15);
    mCameraPitch = 45;
    mCameraYaw = -135;
    mCameraZoom = 30;
    mCameraZoomSensitivity = 0.15;
    mCameraMoveSensitivity = 0.3;
}

QMatrix4x4 MainWindow::getViewMatrix() const
{
    QMatrix4x4 mat;

    mat.translate(0, 0, -mCameraZoom);
    mat.rotate(mCameraPitch, 1, 0);
    mat.rotate(mCameraYaw, 0, 1);
    mat.translate(-mCameraOffset);

    return mat;
}

void MainWindow::updateWind()
{

    float gridSize = 0.15;
    float dt = 0.1;
    float viscosity = 0.00004;
    float density = 3;

    ERROR_IF_FALSE(mWindVelocities1.acquire(mCLWrapper->queue()), "Failed to acquire a CL image.");

    ERROR_IF_FALSE(mWindProgram->updateWindNew(mWindVelocities1.image(),
                                               mCurForce->image(),
                                               gridSize, dt, density, viscosity,
                                               mPressure.image(),
                                               mTemp1.image(),
                                               mTemp2.image()),
                   "Failed to update wind.");

    ERROR_IF_FALSE(mWindVelocities1.release(mCLWrapper->queue()), "Failed to release a CL image.");
}

void MainWindow::updateGrassWindOffsets()
{
    cl_int err;

    err = clEnqueueAcquireGLObjects(mCLWrapper->queue(), 1, &mGrassWindPositions, 0, NULL, NULL);
    ERROR_IF_NOT_SUCCESS(err, "Failed to acquire a GL buffer for OpenCL use.");

    cl_float time = mApplicationStartTime.msecsTo(QTime::currentTime()) / 1000.0;

    ERROR_IF_FALSE(mWindProgram->reactToWind2(mGrassWindPositions,
                                              mGrassPeriodOffsets,
                                              mGrassNormalizedPositions,
                                              mWindVelocities1.image(),
                                              mNumBlades,
                                              time),
                   "Failed to run wind program");

    err = clEnqueueReleaseGLObjects(mCLWrapper->queue(), 1, &mGrassWindPositions, 0, NULL, NULL);
    ERROR_IF_NOT_SUCCESS(err, "Failed to release GL buffers from OpenCL use.");
}

void MainWindow::updateGrassModel(float bendAngle)
{
    QVector<float> model = Grass::makeGrassModel(15, 0.2, 2, bendAngle);
    Q_ASSERT( mNumVerticesPerBlade == 15 * 6 );

    // This assumes that the model takes the same amount of space as before.
    mGrassBladeModelBuffer->bind();
    mGrassBladeModelBuffer->write(0, model.data(), sizeof(float) * model.size());
    mGrassBladeModelBuffer->release();
}

void MainWindow::createGrassModel(float bendAngle)
{
    QVector<float> model = Grass::makeGrassModel(15, 0.2, 2, bendAngle);
    mNumVerticesPerBlade = 15 * 6; // num segments * 6 verts per segment

    mGrassBladeModelBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    mGrassBladeModelBuffer->setUsagePattern(QOpenGLBuffer::StaticDraw);
    ERROR_IF_FALSE(mGrassBladeModelBuffer->create(), "Failed to create model buffer.");

    ERROR_IF_FALSE(mGrassBladeModelBuffer->bind(), "Failed to bind model buffer.");
    mGrassBladeModelBuffer->allocate(model.data(), sizeof(float) * model.size());
    mGrassBladeModelBuffer->release();


    mGrassTexture = new QOpenGLTexture(QImage(":/images/grass_blade.jpg"));
}


void MainWindow::createGrassInstanceData()
{

    const int bladesX = 128;
    const int bladesY = 128;

    mNumBlades = bladesX * bladesY;
    const int offsetsLength = mNumBlades * (3 + 9); // 3 for offset, 9 for rotation matrix
    float *offsets = new float[offsetsLength];

    const int windPositionsLength = mNumBlades * 2;
    const int periodOffsetsLength = mNumBlades;
    const int normalizedPositionsLength = mNumBlades * 2;
    float *windPositions = new float[windPositionsLength];
    float *periodOffsets = new float[periodOffsetsLength];
    float *normalizedPositions = new float[normalizedPositionsLength];

    // Strings together all the even bits of idx.
    auto zOrderX = [] (unsigned int idx) {
        unsigned int x = 0;
        unsigned char shift = 0;

        while (idx > 0)
        {
            x |= ((idx % 2) << shift);

            idx = idx >> 2;

            ++shift;
        }

        return (int)x;
    };

    // Strings together all the odd bits of idx.
    auto zOrderY = [&zOrderX] (unsigned int idx) { return zOrderX(idx >> 1); };

    float positionScale = 0.5;

    for (int index = 0; index < mNumBlades; ++index)
    {
        float xPos = positionScale * (zOrderX(index) - bladesX / 2 + (((float) rand() / RAND_MAX) - 0.5));
        float yPos = positionScale * (zOrderY(index) - bladesY / 2 + (((float) rand() / RAND_MAX) - 0.5));

        float minX = positionScale * (-bladesX / 2 - 2);
        float minY = positionScale * (-bladesY / 2 - 2);
        float rangeX = positionScale * (bladesX + 4);
        float rangeY = positionScale * (bladesY + 4);

        offsets[12*index + 0] = xPos;
        offsets[12*index + 1] = 0;
        offsets[12*index + 2] = yPos;


        float rotation = ((float) rand() / RAND_MAX) * M_PI * 2;
        float c = cos(rotation);
        float s = sin(rotation);

        offsets[12*index +  3] = c;
        offsets[12*index +  4] = 0;
        offsets[12*index +  5] = -s;

        offsets[12*index +  6] = 0;
        offsets[12*index +  7] = 1;
        offsets[12*index +  8] = 0;

        offsets[12*index +  9] = s;
        offsets[12*index + 10] = 0;
        offsets[12*index + 11] = c;

//        float windTime = 4 * M_PI * xPos / bladesX;
        windPositions[2*index + 0] = 0;//yPos/bladesY + 0.9 * sin(windTime);
        windPositions[2*index + 1] = 0;//-xPos/bladesX + 0.5 * sin(windTime);

        periodOffsets[index] = ((float) rand() / RAND_MAX) * 2 * M_PI;

        normalizedPositions[2*index + 0] = ((xPos - minX) / rangeX);
        normalizedPositions[2*index + 1] = ((yPos - minY) / rangeY);
    }



    mGrassBladeInstancedBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    ERROR_IF_FALSE(mGrassBladeInstancedBuffer->create(), "Failed to create offsets buffer.");

    ERROR_IF_FALSE(mGrassBladeInstancedBuffer->bind(), "Failed to bind offsets buffer.");
    mGrassBladeInstancedBuffer->allocate(offsets, sizeof(float) * offsetsLength);
    mGrassBladeInstancedBuffer->release();


    mGrassBladeWindPositionBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    ERROR_IF_FALSE(mGrassBladeWindPositionBuffer->create(), "Failed to create wind position buffer.");

    ERROR_IF_FALSE(mGrassBladeWindPositionBuffer->bind(), "Failed to bind wind position buffer.");
    mGrassBladeWindPositionBuffer->allocate(windPositions, sizeof(float) * windPositionsLength);


    cl_int err;
    mGrassPeriodOffsets = clCreateBuffer(mCLWrapper->context(),
                                         CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                         sizeof(float) * periodOffsetsLength,
                                         periodOffsets,
                                         &err);

    mGrassNormalizedPositions = clCreateBuffer(mCLWrapper->context(),
                                               CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                               sizeof(float) * normalizedPositionsLength,
                                               normalizedPositions,
                                               &err);

    ERROR_IF_NOT_SUCCESS(err, "Failed to create OpenCL buffers.");


    delete [] offsets;
    delete [] windPositions;
    delete [] periodOffsets;
    delete [] normalizedPositions;
}

void MainWindow::createGrassVAO()
{
    mGrassVAO = new QOpenGLVertexArrayObject(this);
    ERROR_IF_FALSE(mGrassVAO->create(), "Failed to create VAO.");
    mGrassVAO->bind();

    mGrassProgram.enableAllAttributeArrays();

    // vPosition, vTexCoord
    mGrassBladeModelBuffer->bind();
    mGrassProgram.setPositionBuffer(0, 5 * sizeof(float));
    mGrassProgram.setTexCoordBuffer(3 * sizeof(float), 5 * sizeof(float));
    mGrassBladeModelBuffer->release();

    // vOffset, vRotation
    mGrassBladeInstancedBuffer->bind();
    mGrassProgram.setOffsetBuffer(0, 12 * sizeof(float));
    mGrassProgram.setRotationBuffer(3 * sizeof(float), 12 * sizeof(float));
    glVertexAttribDivisor(mGrassProgram.vOffset(), 1);
    glVertexAttribDivisor(mGrassProgram.vRotationCol0(), 1);
    glVertexAttribDivisor(mGrassProgram.vRotationCol1(), 1);
    glVertexAttribDivisor(mGrassProgram.vRotationCol2(), 1);
    mGrassBladeInstancedBuffer->release();

    // vWindPosition
    mGrassBladeWindPositionBuffer->bind();
    mGrassProgram.setWindPositionBuffer();
    glVertexAttribDivisor(mGrassProgram.vWindPosition(), 1);
    mGrassBladeWindPositionBuffer->release();

    mGrassVAO->release();
}

void MainWindow::createCLBuffersFromGLBuffers()
{
    cl_int err;
    mGrassWindPositions = clCreateFromGLBuffer(mCLWrapper->context(),
                                               CL_MEM_READ_WRITE,
                                               mGrassBladeWindPositionBuffer->bufferId(),
                                               &err);
    ERROR_IF_NOT_SUCCESS(err, "Couldn't share grass wind positions buffer.");
}
