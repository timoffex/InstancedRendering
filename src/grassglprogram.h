#ifndef GRASSGLPROGRAM_H
#define GRASSGLPROGRAM_H

#include <QOpenGLShaderProgram>

class GrassGLProgram
{
public:
    GrassGLProgram();
    ~GrassGLProgram();

    /// Creates the program. An OpenGL context must be current.
    /// Returns true on success, false on failure.
    bool create();

    /// Destroys the program.
    void destroy();

    bool bind();
    void release();

    void enableAllAttributeArrays();

    void setPositionBuffer(int offset = 0, int stride = 0);
    void setTexCoordBuffer(int offset = 0, int stride = 0);
    void setOffsetBuffer(int offset = 0, int stride = 0);
    void setRotationBuffer(int offset = 0, int stride = 0);     /// Assumes column-major layout.
    void setWindPositionBuffer(int offset = 0, int stride = 0);

    void setMVP(const QMatrix4x4 &mvp);
    void setGrassTextureUnit(int textureUnit);


    int vPosition() const       { return attr_vPosition; }
    int vTexCoord() const       { return attr_vTexCoord; }
    int vOffset() const         { return attr_vOffset; }
    int vRotationCol0() const   { return attr_vRotation+0; }
    int vRotationCol1() const   { return attr_vRotation+1; }
    int vRotationCol2() const   { return attr_vRotation+2; }
    int vWindPosition() const   { return attr_vWindPosition; }

    int uMVP() const            { return unif_uMVP; }
    int uGrassTexture() const   { return unif_uGrassTexture; }

private:
    bool mCreated;

    QOpenGLShaderProgram *mProgram;

    int attr_vPosition;
    int attr_vTexCoord;
    int attr_vOffset;
    int attr_vRotation;
    int attr_vWindPosition;

    int unif_uMVP;
    int unif_uGrassTexture;
};

#endif // GRASSGLPROGRAM_H
