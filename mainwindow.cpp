#include "mainwindow.h"


MainWindow::MainWindow(QWindow *parent)
    : QOpenGLWindow(NoPartialUpdate, parent)
{
}

MainWindow::~MainWindow()
{
    // mGrassProgram has this as the parent
    delete mGrassBladeModelBuffer;
}

static void ERROR(const char *msg)
{
    qDebug() << "ERROR: " << msg;
    exit(1);
}

void MainWindow::initializeGL()
{
    initializeOpenGLFunctions();

    mGrassProgram = new QOpenGLShaderProgram(this);

    mGrassProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/grass.vert");
    mGrassProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/grass.frag");

    if (!mGrassProgram->link() || !mGrassProgram->isLinked())
        ERROR( "Program didn't link!" );

    mGrassProgram_vPosition = mGrassProgram->attributeLocation("vPosition");
    mGrassProgram_uMVP = mGrassProgram->uniformLocation("uMVP");

    if (mGrassProgram_vPosition == -1)
        ERROR( "Didn't find vPosition." );

    if (mGrassProgram_uMVP == -1)
        ERROR( "Didn't find uMVP." );



    /* First test:
        Use just the first buffer to store one blade of grass.
        Initialize the VAO.*/

    int grassDataLength = 9;
    float grassData[] = {
         0.0,  0.0,  0.0,
         0.0,  1.0,  0.0,
         1.0,  1.0,  0.0
    };

    mGrassBladeModelBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    if (!mGrassBladeModelBuffer->create())
        ERROR( "Failed to create model buffer." );

    if (!mGrassBladeModelBuffer->bind())
        ERROR( "Failed to bind model buffer." );

    mGrassBladeModelBuffer->allocate(grassData, sizeof(float) * grassDataLength);
    mGrassBladeModelBuffer->release();


    mGrassVAO = new QOpenGLVertexArrayObject(this);
    if (!mGrassVAO->create())
        ERROR( "Failed to create VAO." );
    mGrassVAO->bind();
    mGrassBladeModelBuffer->bind();
    mGrassProgram->setAttributeBuffer(mGrassProgram_vPosition, GL_FLOAT, 0, 3);
    mGrassBladeModelBuffer->release();
    mGrassVAO->release();

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

    glClear(GL_COLOR_BUFFER_BIT);

    QMatrix4x4 matrix;
    matrix.perspective(90, (float) width() / height(), 0.1, 100);
    matrix.translate(0, 0, -2);
    matrix.rotate(-30, 1, 0);

    if (!mGrassProgram->bind())
        ERROR( "Failed to bind program in paint call." );

    mGrassProgram->setUniformValue(mGrassProgram_uMVP, matrix);

    mGrassVAO->bind();
    mGrassProgram->enableAttributeArray(mGrassProgram_vPosition);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    mGrassProgram->disableAttributeArray(mGrassProgram_vPosition);
    mGrassVAO->release();

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
