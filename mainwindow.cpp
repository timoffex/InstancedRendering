#include "mainwindow.h"

// For rand()
#include <cstdlib>

MainWindow::MainWindow(QWindow *parent)
    : QOpenGLWindow(NoPartialUpdate, parent)
{
}

MainWindow::~MainWindow()
{
    // shader programs and VAOs have this object as the parent

    delete mGrassBladeModelBuffer;
    delete mGrassBladeOffsetsInstancedBuffer;

    // TODO: This, and probably the above deletes, must happen with a current context.
//    delete mGrassTexture;
}

static void ERROR(const char *msg)
{
    qDebug() << "ERROR: " << msg;
    exit(1);
}

void MainWindow::initializeGL()
{
    initializeOpenGLFunctions();

    // TODO enable face culling
    glEnable(GL_DEPTH_TEST);

    mGrassProgram = new QOpenGLShaderProgram(this);

    mGrassProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/grass.vert");
    mGrassProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/grass.frag");

    if (!mGrassProgram->link() || !mGrassProgram->isLinked())
        ERROR( "Program didn't link!" );

    mGrassProgram_vPosition = mGrassProgram->attributeLocation("vPosition");
    mGrassProgram_vTexCoord = mGrassProgram->attributeLocation("vTexCoord");
    mGrassProgram_vOffset = mGrassProgram->attributeLocation("vOffset");
    mGrassProgram_uMVP = mGrassProgram->uniformLocation("uMVP");
    mGrassProgram_uGrassTexture = mGrassProgram->uniformLocation("uGrassTexture");

    if (mGrassProgram_vPosition == -1)
        ERROR( "Didn't find vPosition." );

    if (mGrassProgram_vOffset == -1)
        ERROR( "Didn't find vTexCoord." );

    if (mGrassProgram_vOffset == -1)
        ERROR( "Didn't find vOffset." );

    if (mGrassProgram_uMVP == -1)
        ERROR( "Didn't find uMVP." );

    if (mGrassProgram_uGrassTexture == -1)
        ERROR( "Didn't find uGrassTexture." );



    createGrassModel();
    createGrassOffsets();
    createGrassVAO();

    if (checkGLErrors())
        qDebug() << "^ in initializeGL()";
}

void MainWindow::resizeGL(int w, int h)
{
    // Do nothing for now. Later, use this to change up projection matrix when aspect changes.
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void MainWindow::paintGL()
{
    glViewport(0, 0, width(), height());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 matrix;
    matrix.perspective(90, (float) width() / height(), 0.1, 100);
    matrix.translate(0, 0, -6);
    matrix.rotate(30, 1, 0);
    matrix.rotate(mRotation, 0, 1);
    mRotation += 1;

    if (!mGrassProgram->bind())
        ERROR( "Failed to bind program in paint call." );

    mGrassProgram->setUniformValue(mGrassProgram_uMVP, matrix);
    mGrassProgram->setUniformValue(mGrassProgram_uGrassTexture, 0); // uses texture in texture unit 0

    glActiveTexture(GL_TEXTURE0);
    mGrassTexture->bind(); // binds to texture unit 0

    mGrassVAO->bind();
    glDrawArraysInstanced(GL_TRIANGLES, 0, mNumVerticesPerBlade, mNumBlades);
    mGrassVAO->release();

    mGrassTexture->release();

    mGrassProgram->release();


    if (checkGLErrors())
        qDebug() << "^ in paintGL() at end";
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



void MainWindow::createGrassModel()
{
    mNumVerticesPerBlade = 6;
    int grassDataLength = 5 * mNumVerticesPerBlade;
    float grassData[] = {
         -0.1,  0.0,  0.0,   0.0,  1.0,
          0.1,  0.0,  0.0,   0.0,  0.0,
          0.1,  1.0,  0.0,   1.0,  0.0,

         -0.1,  0.0,  0.0,   0.0,  1.0,
          0.1,  1.0,  0.0,   1.0,  0.0,
         -0.1,  1.0,  0.0,   1.0,  1.0,
    };

    mGrassBladeModelBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    if (!mGrassBladeModelBuffer->create())
        ERROR( "Failed to create model buffer." );

    if (!mGrassBladeModelBuffer->bind())
        ERROR( "Failed to bind model buffer." );

    mGrassBladeModelBuffer->allocate(grassData, sizeof(float) * grassDataLength);
    mGrassBladeModelBuffer->release();


    mGrassTexture = new QOpenGLTexture(QImage(":/images/grass_blade.jpg"));
}


void MainWindow::createGrassOffsets()
{

    const int bladesX = 100;
    const int bladesY = 100;

    mNumBlades = bladesX * bladesY;
    const int offsetsLength = mNumBlades * 3;
    float *offsets = new float[offsetsLength];

    for (int x = 0; x < bladesX; ++x)
    {
        for (int y = 0; y < bladesY; ++y)
        {
            int index = y + x * bladesY;

            float xPos = (((float) rand() / RAND_MAX) - 0.5) * bladesX;
            float yPos = (((float) rand() / RAND_MAX) - 0.5) * bladesY;

            offsets[3*index + 0] = xPos;
            offsets[3*index + 1] = 0;
            offsets[3*index + 2] = yPos;

        }
    }



    mGrassBladeOffsetsInstancedBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);

    if (!mGrassBladeOffsetsInstancedBuffer->create())
        ERROR( "Failed to create offsets buffer." );

    if (!mGrassBladeOffsetsInstancedBuffer->bind())
        ERROR( "Failed to bind offsets buffer." );

    mGrassBladeOffsetsInstancedBuffer->allocate(offsets, sizeof(float) * offsetsLength);
    mGrassBladeOffsetsInstancedBuffer->release();

    delete [] offsets;
}

void MainWindow::createGrassVAO()
{
    mGrassVAO = new QOpenGLVertexArrayObject(this);
    if (!mGrassVAO->create())
        ERROR( "Failed to create VAO." );
    mGrassVAO->bind();

    mGrassProgram->enableAttributeArray(mGrassProgram_vPosition);
    mGrassProgram->enableAttributeArray(mGrassProgram_vTexCoord);
    mGrassProgram->enableAttributeArray(mGrassProgram_vOffset);

    mGrassBladeModelBuffer->bind();
    mGrassProgram->setAttributeBuffer(mGrassProgram_vPosition, GL_FLOAT, 0, 3, 5 * sizeof(float));
    mGrassProgram->setAttributeBuffer(mGrassProgram_vTexCoord, GL_FLOAT, 3 * sizeof(float), 2, 5 * sizeof(float));
    mGrassBladeModelBuffer->release();

    mGrassBladeOffsetsInstancedBuffer->bind();
    mGrassProgram->setAttributeBuffer(mGrassProgram_vOffset, GL_FLOAT, 0, 3);
    glVertexAttribDivisor(mGrassProgram_vOffset, 1);
    mGrassBladeOffsetsInstancedBuffer->release();

    mGrassVAO->release();
}
