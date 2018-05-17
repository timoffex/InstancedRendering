#include "windquadglprogram.h"

WindQuadGLProgram::WindQuadGLProgram()
    : mCreated(false)
{
}


WindQuadGLProgram::~WindQuadGLProgram()
{
    delete mProgram;
}

bool WindQuadGLProgram::create()
{
    mProgram = new QOpenGLShaderProgram();

    mProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/texturedNDC.vert");
    mProgram->addCacheableShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/windQuad.frag");

    if (!mProgram->link())
    {
        qDebug() << "WindQuad program failed to link.";
        return false;
    }

    attr_vPosition = mProgram->attributeLocation("vPosition");
    attr_vTexCoord = mProgram->attributeLocation("vTexCoord");
    unif_uWindSpeed = mProgram->uniformLocation("uWindSpeed");

#ifndef RET_FALSE_IF_NEG1
#define RET_FALSE_IF_NEG1(x) if(x==-1) { qDebug() << "Couldn't find " #x; return false; }

    RET_FALSE_IF_NEG1(attr_vPosition);
    RET_FALSE_IF_NEG1(attr_vTexCoord);
    RET_FALSE_IF_NEG1(unif_uWindSpeed);
#else
    static_assert(false);
#endif

    mCreated = true;
    return true;
}


bool WindQuadGLProgram::bind()
{
    Q_ASSERT( mCreated );
    if (!mProgram->bind())
        return false;
    return true;
}

void WindQuadGLProgram::release()
{
    Q_ASSERT( mCreated );
    mProgram->release();
}

void WindQuadGLProgram::enableAllAttributeArrays()
{
    Q_ASSERT( mCreated );
    mProgram->enableAttributeArray(attr_vPosition);
    mProgram->enableAttributeArray(attr_vTexCoord);
}

void WindQuadGLProgram::setPositionBuffer(int offset, int stride)
{
    Q_ASSERT( mCreated );
    mProgram->setAttributeBuffer(attr_vPosition, GL_FLOAT, offset, 2, stride);
}

void WindQuadGLProgram::setTexCoordBuffer(int offset, int stride)
{
    Q_ASSERT( mCreated );
    mProgram->setAttributeBuffer(attr_vTexCoord, GL_FLOAT, offset, 2, stride);
}


void WindQuadGLProgram::setWindSpeedTextureUnit(int textureUnit)
{
    Q_ASSERT( mCreated );
    mProgram->setUniformValue(unif_uWindSpeed, textureUnit);
}
