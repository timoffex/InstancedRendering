#ifndef WINDQUADPROGRAM_H
#define WINDQUADPROGRAM_H

#include <QOpenGLShaderProgram>

class WindQuadGLProgram
{
public:
    WindQuadGLProgram();
    ~WindQuadGLProgram();

    bool create();
    void destroy();

    bool bind();
    void release();

    void enableAllAttributeArrays();

    void setPositionBuffer(int offset = 0, int stride = 0);
    void setTexCoordBuffer(int offset = 0, int stride = 0);

    void setWindSpeedTextureUnit(int textureUnit);

private:
    bool mCreated;
    QOpenGLShaderProgram *mProgram;

    int attr_vPosition;
    int attr_vTexCoord;
    int unif_uWindSpeed;
};

#endif // WINDQUADPROGRAM_H
