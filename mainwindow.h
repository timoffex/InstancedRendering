#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "myclwrapper.h"
#include "grasswindclprogram.h"
#include "grassglprogram.h"

#include <QOpenGLWindow>
#include <QOpenGLExtraFunctions>

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>

#include <QMouseEvent>
#include <QPoint>

#include <QTime>

class MainWindow : public QOpenGLWindow, QOpenGLExtraFunctions
{
    Q_OBJECT

public:
    MainWindow(QWindow *parent = 0);
    ~MainWindow();


protected:
    /* From QOpenGLWindow */
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mouseMoveEvent(QMouseEvent *evt) override;
    void mousePressEvent(QMouseEvent *evt) override;
    void mouseReleaseEvent(QMouseEvent *evt) override;


    bool checkGLErrors();

private:

    void updateWind();
    void updateGrassWindOffsets();

    void updateGrassModel(float bendAngle);
    void createGrassModel(float bendAngle);
    void createGrassInstanceData();
    void createGrassVAO();

    void createCLBuffersFromGLBuffers();

    /// Stores the data for a single grass blade.
    QOpenGLBuffer *mGrassBladeModelBuffer;

    /// The per-instance data for grass blades.
    QOpenGLBuffer *mGrassBladeInstancedBuffer;

    /// The "wind position" of each grass blade.
    /// Shared by OpenCL and OpenGL.
    QOpenGLBuffer *mGrassBladeWindPositionBuffer;

    /// The VAO holding all grass data.
    QOpenGLVertexArrayObject *mGrassVAO;

    /// The grass texture.
    QOpenGLTexture *mGrassTexture;


    /// The shader program that should be used for rendering grass.
    GrassGLProgram mGrassProgram;

    /// The number of grass blade instances that should be drawn.
    int mNumBlades;

    /// The number of vertices in the grass blade model.
    int mNumVerticesPerBlade;


    /// The OpenCL program wrapper for the wind computations.
    GrassWindCLProgram *mWindProgram;

    /// Wrapper for a CL device, context & command queue.
    MyCLWrapper *mCLWrapper;


    cl_mem mGrassWindPositions;     /// "position" of each grass blade (used for wind effect)
    cl_mem mGrassWindVelocities;    /// "velocity" of each grass blade (used for wind effect)
    cl_mem mGrassWindStrength;      /// "strength" of the wind for a grass blade

    /// Variable used for rotating the screen.
    float mRotation = 0;
    QPoint mDragStart;
    float mDragRotationStart;
    bool mDragging = false;
};

#endif // MAINWINDOW_H
