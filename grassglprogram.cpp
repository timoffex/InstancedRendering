#include "grassglprogram.h"

#include <QOpenGLContext>


GrassGLProgram::GrassGLProgram()
    : mCreated(false)
{
}


GrassGLProgram::~GrassGLProgram()
{
    // Qt documentation is a little unclear on whether this
    // actually releases whatever resources are associated
    // to the program and whether a context need be current.
    delete mProgram;
}


bool GrassGLProgram::create()
{
    Q_ASSERT( QOpenGLContext::currentContext() != nullptr );

    mProgram = new QOpenGLShaderProgram();

    mProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/grass.vert");
    mProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/grass.frag");

    if (!mProgram->link())
    {
        qDebug() << "Program failed to link.";
        return false;
    }


    attr_vPosition = mProgram->attributeLocation("vPosition");
    attr_vTexCoord = mProgram->attributeLocation("vTexCoord");
    attr_vOffset = mProgram->attributeLocation("vOffset");
    attr_vRotation = mProgram->attributeLocation("vRotation");
    attr_vWindPosition = mProgram->attributeLocation("vWindPosition");

    unif_uMVP = mProgram->uniformLocation("uMVP");
    unif_uGrassTexture = mProgram->uniformLocation("uGrassTexture");


#ifndef RET_FALSE_ON_NEG1
#define RET_FALSE_ON_NEG1(v) if (v == -1) { qDebug() << #v " not found in grass program."; return false; }

    RET_FALSE_ON_NEG1(attr_vPosition);
    RET_FALSE_ON_NEG1(attr_vTexCoord);
    RET_FALSE_ON_NEG1(attr_vOffset);
    RET_FALSE_ON_NEG1(attr_vRotation);
    RET_FALSE_ON_NEG1(attr_vWindPosition);

    RET_FALSE_ON_NEG1(unif_uMVP);
    RET_FALSE_ON_NEG1(unif_uGrassTexture);
#undef RET_FALSE_ON_NEG1
#else
    // Alert the programmer that his/her symbols are being overwritten.
    static_assert(false);
#endif

    mCreated = true;
    return true;
}


bool GrassGLProgram::bind()
{
    Q_ASSERT( mCreated );
    return mProgram->bind();
}

void GrassGLProgram::release()
{
    Q_ASSERT( mCreated );
    mProgram->release();
}

void GrassGLProgram::enableAllAttributeArrays()
{
    mProgram->enableAttributeArray(attr_vPosition);
    mProgram->enableAttributeArray(attr_vTexCoord);
    mProgram->enableAttributeArray(attr_vOffset);
    mProgram->enableAttributeArray(attr_vRotation+0);
    mProgram->enableAttributeArray(attr_vRotation+1);
    mProgram->enableAttributeArray(attr_vRotation+2);
    mProgram->enableAttributeArray(attr_vWindPosition);
}

void GrassGLProgram::setPositionBuffer(int offset, int stride)
{
    Q_ASSERT( mCreated );
    mProgram->setAttributeBuffer(attr_vPosition, GL_FLOAT, offset, 3, stride);
}

void GrassGLProgram::setTexCoordBuffer(int offset, int stride)
{
    Q_ASSERT( mCreated );
    mProgram->setAttributeBuffer(attr_vTexCoord, GL_FLOAT, offset, 2, stride);
}

void GrassGLProgram::setOffsetBuffer(int offset, int stride)
{
    Q_ASSERT( mCreated );
    mProgram->setAttributeBuffer(attr_vOffset, GL_FLOAT, offset, 3, stride);
}

void GrassGLProgram::setRotationBuffer(int offset, int stride)
{
    Q_ASSERT( mCreated );
    mProgram->setAttributeBuffer(attr_vRotation+0, GL_FLOAT, offset + 0*sizeof(float), 3, stride);
    mProgram->setAttributeBuffer(attr_vRotation+1, GL_FLOAT, offset + 3*sizeof(float), 3, stride);
    mProgram->setAttributeBuffer(attr_vRotation+2, GL_FLOAT, offset + 6*sizeof(float), 3, stride);
}

void GrassGLProgram::setWindPositionBuffer(int offset, int stride)
{
    Q_ASSERT( mCreated );
    mProgram->setAttributeBuffer(attr_vWindPosition, GL_FLOAT, offset, 2, stride);
}

void GrassGLProgram::setMVP(const QMatrix4x4 &mvp)
{
    Q_ASSERT( mCreated );
    mProgram->setUniformValue(unif_uMVP, mvp);
}

void GrassGLProgram::setGrassTextureUnit(int textureUnit)
{
    Q_ASSERT( mCreated );
    mProgram->setUniformValue(unif_uGrassTexture, textureUnit);
}
