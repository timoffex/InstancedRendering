#ifndef IMAGETESTWINDOW_H
#define IMAGETESTWINDOW_H

#include "cl_interface/myclimage_rgba32f.h"
#include "cl_interface/myclimagedescriptor.h"
#include "cl_interface/myclwrapper.h"

#include "windquadglprogram.h"

// This class will be tested.
#include "grasswindclprogram.h"


#include <QOpenGLWindow>
#include <QOpenGLFunctions>

#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>


class ImageTestWindow : public QOpenGLWindow, QOpenGLFunctions
{
public:
    ImageTestWindow();

protected:
    /* From QOpenGLWindow */
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;


private:

    void doUpdate();

    void createScreenQuadProgram();
    void createScreenQuadData();
    void createScreenQuadTexture();

    void initializeCL();

    bool checkGLErrors();

    WindQuadGLProgram *mScreenQuadProgram;
    QOpenGLBuffer *mScreenQuadBuffer;
    QOpenGLVertexArrayObject *mScreenQuadVAO;
    QOpenGLTexture *mWindTexture;
    QOpenGLTexture *mPressureTexture;


    MyCLWrapper mCLWrapper;

    GrassWindCLProgram mWindCLProgram;

    MyCLImage_RGBA32F mWindSpeeds;
    MyCLImage_RGBA32F mForces;
    MyCLImage_RGBA32F mPressure;
    MyCLImage_RGBA32F mTemp1;
    MyCLImage_RGBA32F mTemp2;
};

#endif // IMAGETESTWINDOW_H
