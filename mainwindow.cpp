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

static void ERROR_IF_NEG1(int num, const char *msg)
{
    if (num == -1)
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

    mGrassProgram = new QOpenGLShaderProgram(this);

    mGrassProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/grass.vert");
    mGrassProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/grass.frag");

    ERROR_IF_FALSE(mGrassProgram->link() && mGrassProgram->isLinked(), "Program didn't link!");

    mGrassProgram_vPosition = mGrassProgram->attributeLocation("vPosition");
    mGrassProgram_vTexCoord = mGrassProgram->attributeLocation("vTexCoord");
    mGrassProgram_vOffset = mGrassProgram->attributeLocation("vOffset");
    mGrassProgram_vRotation = mGrassProgram->attributeLocation("vRotation");
    mGrassProgram_vWindPosition = mGrassProgram->attributeLocation("vWindPosition");
    mGrassProgram_uMVP = mGrassProgram->uniformLocation("uMVP");
    mGrassProgram_uGrassTexture = mGrassProgram->uniformLocation("uGrassTexture");

    ERROR_IF_NEG1(mGrassProgram_vPosition, "Didn't find vPosition.");
    ERROR_IF_NEG1(mGrassProgram_vTexCoord, "Didn't find vTexCoord.");
    ERROR_IF_NEG1(mGrassProgram_vOffset, "Didn't find vOffset.");
    ERROR_IF_NEG1(mGrassProgram_vRotation, "Didn't find vRotation.");
    ERROR_IF_NEG1(mGrassProgram_vWindPosition, "Didn't find vWindPosition.");
    ERROR_IF_NEG1(mGrassProgram_uMVP, "Didn't find uMVP.");
    ERROR_IF_NEG1(mGrassProgram_uGrassTexture, "Didn't find uGrassTexture.");


    mGrassBend = 45;
    createGrassModel(mGrassBend);
    createGrassOffsets();
    createGrassVAO();

    if (checkGLErrors())
        qDebug() << "^ in initializeGL()";


    // Initialize OpenCL stuff here.
    mCLWrapper = new MyCLWrapper();
    ERROR_IF_FALSE(mCLWrapper->create(), "Failed to initialize OpenCL.");

    mWindProgram = new GrassWindCLProgram();
    ERROR_IF_FALSE(mWindProgram->create(mCLWrapper), "Failed to create wind program.");

    cl_int err;
    mGrassWindPositions = clCreateFromGLBuffer(mCLWrapper->context(),
                                               CL_MEM_READ_WRITE,
                                               mGrassBladeWindPositionBuffer->bufferId(),
                                               &err);
    ERROR_IF_NOT_SUCCESS(err, "Couldn't share grass wind positions buffer.");


    const int windVelocitiesLength = mNumBlades;
    const int windStrengthsLength = mNumBlades;
    float *windVelocities = new float[windVelocitiesLength];
    float *windStrengths = new float[windStrengthsLength];

    for (int bladeIdx = 0; bladeIdx < mNumBlades; ++bladeIdx)
    {
        windVelocities[bladeIdx] = 0;
        windStrengths[bladeIdx] = 1;
    }

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
}

void MainWindow::resizeGL(int w, int h)
{
    // Do nothing for now. Later, use this to change up projection matrix when aspect changes.
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void MainWindow::paintGL()
{
    // Update grass model. (WHEN UNCOMMENTING, REMEMBER TO UNCOMMENT update() AT END).
//    mGrassBend = 80 * cos(mStartTime.msecsTo(QTime::currentTime()) / 700.0);
//    updateGrassModel(mGrassBend);

//    float cosTime = cos(mStartTime.msecsTo(QTime::currentTime()) / 600.0);
//    float cosSquared = cosTime * cosTime;
//    mWindStrength = 0.25 * cosSquared + 0.2;

    /* Run the wind simulation. */

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

    /* End wind simulation. */


    glViewport(0, 0, width(), height());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 matrix;
    matrix.perspective(90, (float) width() / height(), 0.1, 100);
    matrix.translate(0, 0, -20);
    matrix.rotate(30, 1, 0);
    matrix.rotate(mRotation, 0, 1);

    ERROR_IF_FALSE(mGrassProgram->bind(), "Failed to bind program in paint call.");

    mGrassProgram->setUniformValue(mGrassProgram_uMVP, matrix);
    mGrassProgram->setUniformValue(mGrassProgram_uGrassTexture, 0); // uses texture in texture unit 0
//    mGrassProgram->setUniformValue(mGrassProgram_uWindStrength, mWindStrength);

    glActiveTexture(GL_TEXTURE0);
    mGrassTexture->bind(); // binds to texture unit 0

    mGrassVAO->bind();
    glDrawArraysInstanced(GL_TRIANGLES, 0, mNumVerticesPerBlade, mNumBlades);
    mGrassVAO->release();

    mGrassTexture->release();

    mGrassProgram->release();


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
    mStartTime = QTime::currentTime();
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


void MainWindow::createGrassOffsets()
{

    const int bladesX = 100;
    const int bladesY = 100;

    mNumBlades = bladesX * bladesY;
    const int offsetsLength = mNumBlades * (3 + 9); // 3 for offset, 9 for rotation matrix
    float *offsets = new float[offsetsLength];

    const int windPositionsLength = mNumBlades;
    float *windPositions = new float[windPositionsLength];

    for (int x = 0; x < bladesX; ++x)
    {
        for (int y = 0; y < bladesY; ++y)
        {
            int index = y + x * bladesY;

            float xPos = (((float) rand() / RAND_MAX) - 0.5) * bladesX;
            float yPos = (((float) rand() / RAND_MAX) - 0.5) * bladesY;

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

            windPositions[index] = 3 * (0.5 + xPos / bladesX);
        }
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

    delete [] offsets;
    delete [] windPositions;
}

void MainWindow::createGrassVAO()
{
    mGrassVAO = new QOpenGLVertexArrayObject(this);
    ERROR_IF_FALSE(mGrassVAO->create(), "Failed to create VAO.");
    mGrassVAO->bind();

    mGrassProgram->enableAttributeArray(mGrassProgram_vPosition);
    mGrassProgram->enableAttributeArray(mGrassProgram_vTexCoord);
    mGrassProgram->enableAttributeArray(mGrassProgram_vOffset);

    // A mat3 is like 3 vec3 attributes.
    mGrassProgram->enableAttributeArray(mGrassProgram_vRotation + 0);
    mGrassProgram->enableAttributeArray(mGrassProgram_vRotation + 1);
    mGrassProgram->enableAttributeArray(mGrassProgram_vRotation + 2);

    mGrassProgram->enableAttributeArray(mGrassProgram_vWindPosition);

    mGrassBladeModelBuffer->bind();
    mGrassProgram->setAttributeBuffer(mGrassProgram_vPosition, GL_FLOAT, 0, 3, 5 * sizeof(float));
    mGrassProgram->setAttributeBuffer(mGrassProgram_vTexCoord, GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));
    mGrassBladeModelBuffer->release();

    mGrassBladeInstancedBuffer->bind();
    mGrassProgram->setAttributeBuffer(mGrassProgram_vOffset, GL_FLOAT, 0, 3, 12*sizeof(float));
    mGrassProgram->setAttributeBuffer(mGrassProgram_vRotation+0, GL_FLOAT, 3*sizeof(float), 3, 12*sizeof(float));
    mGrassProgram->setAttributeBuffer(mGrassProgram_vRotation+1, GL_FLOAT, 6*sizeof(float), 3, 12*sizeof(float));
    mGrassProgram->setAttributeBuffer(mGrassProgram_vRotation+2, GL_FLOAT, 9*sizeof(float), 3, 12*sizeof(float));
    glVertexAttribDivisor(mGrassProgram_vOffset, 1);
    glVertexAttribDivisor(mGrassProgram_vRotation+0, 1);
    glVertexAttribDivisor(mGrassProgram_vRotation+1, 1);
    glVertexAttribDivisor(mGrassProgram_vRotation+2, 1);
    mGrassBladeInstancedBuffer->release();

    mGrassBladeWindPositionBuffer->bind();
    mGrassProgram->setAttributeBuffer(mGrassProgram_vWindPosition, GL_FLOAT, 0, 1);
    glVertexAttribDivisor(mGrassProgram_vWindPosition, 1);
    mGrassBladeWindPositionBuffer->release();

    mGrassVAO->release();
}
