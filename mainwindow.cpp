#include "mainwindow.h"

#include "grass.h"

// For rand()
#include <cstdlib>

#include <QtMath>

MainWindow::MainWindow(QWindow *parent)
    : QOpenGLWindow(NoPartialUpdate, parent)
{
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

    glViewport(0, 0, width(), height());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 matrix;
    matrix.perspective(90, (float) width() / height(), 0.1, 100);
    matrix.translate(0, 0, -20);
    matrix.rotate(30, 1, 0);
    matrix.rotate(mRotation, 0, 1);

    ERROR_IF_FALSE(mGrassProgram.bind(), "Failed to bind program in paint call.");

    mGrassProgram.setMVP(matrix);
    mGrassProgram.setGrassTextureUnit(0);

    glActiveTexture(GL_TEXTURE0);
    mGrassTexture->bind(); // binds to texture unit 0

    mGrassVAO->bind();
    glDrawArraysInstanced(GL_TRIANGLES, 0, mNumVerticesPerBlade, mNumBlades);
    mGrassVAO->release();

    mGrassTexture->release();

    mGrassProgram.release();


    if (checkGLErrors())
        qDebug() << "^ in paintGL() at end";

    // Schedule another update.
    update();
}


void MainWindow::mouseMoveEvent(QMouseEvent *evt)
{
    if (mDragging)
    {
        mRotation = mDragRotationStart + (evt->x() - mDragStart.x()) / 5.0;
        update();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *evt)
{
    if (evt->button() == Qt::LeftButton)
    {
        Q_ASSERT( !mDragging );
        mDragging = true;
        mDragStart = evt->pos();
        mDragRotationStart = mRotation;
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *evt)
{
    if (evt->button() == Qt::LeftButton)
    {
        mDragging = false;
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
            qDebug() << "Unprocessed error.";
            break;
        }

        hadError = true;
    }

    return hadError;
}



void MainWindow::updateWind()
{
    // TODO
}

void MainWindow::updateGrassWindOffsets()
{
    cl_int err;

    err = clEnqueueAcquireGLObjects(mCLWrapper->queue(), 1, &mGrassWindPositions, 0, NULL, NULL);
    ERROR_IF_NOT_SUCCESS(err, "Failed to acquire a GL buffer for OpenCL use.");


    ERROR_IF_FALSE(mWindProgram->reactToWind(mGrassWindPositions,
                                             mGrassWindVelocities,
                                             mGrassWindStrength, mNumBlades, 0.1),
                   "Failed to run wind program");

    err = clEnqueueReleaseGLObjects(mCLWrapper->queue(), 1, &mGrassWindPositions, 0, NULL, NULL);
    ERROR_IF_NOT_SUCCESS(err, "Failed to release GL buffers from OpenCL use.");

    err = clFinish(mCLWrapper->queue());
    ERROR_IF_NOT_SUCCESS(err, "Failed to finish OpenCL commands.");
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
    const int windVelocitiesLength = mNumBlades * 2;
    const int windStrengthsLength = mNumBlades * 2;
    float *windPositions = new float[windPositionsLength];
    float *windVelocities = new float[windVelocitiesLength];
    float *windStrengths = new float[windStrengthsLength];

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

    auto zOrderY = [&zOrderX] (unsigned int idx) { return zOrderX(idx >> 1); };

    for (int index = 0; index < mNumBlades; ++index)
    {
        float xPos = zOrderX(index) - bladesX / 2 + (((float) rand() / RAND_MAX) - 0.5);
        float yPos = zOrderY(index) - bladesY / 2 + (((float) rand() / RAND_MAX) - 0.5);

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

        float windTime = 4 * M_PI * xPos / bladesX;
        windPositions[2*index + 0] = yPos/bladesY + 0.9 * sin(windTime);
        windPositions[2*index + 1] = -xPos/bladesX + 0.5 * sin(windTime);

        windVelocities[2*index + 0] = 0 + 0.9 * cos(windTime);
        windVelocities[2*index + 1] = 0 + 0.5 * cos(windTime);

        windStrengths[2*index + 0] = yPos / bladesY;
        windStrengths[2*index + 1] = -xPos / bladesX;
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
    mGrassWindVelocities = clCreateBuffer(mCLWrapper->context(),
                                          CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                          sizeof(float) * windVelocitiesLength,
                                          windVelocities,
                                          &err);

    mGrassWindStrength = clCreateBuffer(mCLWrapper->context(),
                                        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                        sizeof(float) * windStrengthsLength,
                                        windStrengths,
                                        &err);

    ERROR_IF_NOT_SUCCESS(err, "Failed to create OpenCL buffers.");


    delete [] offsets;
    delete [] windPositions;
    delete [] windStrengths;
    delete [] windVelocities;
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
