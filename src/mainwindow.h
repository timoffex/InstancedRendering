#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "cl_interface/myclwrapper.h"
#include "cl_interface/myclimage.h"

#include "fluid2dsimulation.h"
#include "grasswindclprogram.h"
#include "grassglprogram.h"
#include "windquadglprogram.h"

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
    void wheelEvent(QWheelEvent *evt) override;

    void keyReleaseEvent(QKeyEvent *evt) override;


    bool checkGLErrors();

private:

    void releaseAllResources();
    void releaseCLResources();
    void releaseGLResources();

    void beginDrag(QMouseEvent *evt);

    void initializeCamera();
    QMatrix4x4 getViewMatrix() const;


    void drawGrass();
    void drawWindQuad();


    void updateWind();
    void updateGrassWindOffsets();

    void updateGrassModel(float bendAngle);
    void createGrassModel(float bendAngle);
    void createGrassInstanceData();
    void createGrassVAO();

    void createCLBuffersFromGLBuffers();
    void createWindSimulation();
    void createWindQuadData();


    /// Whether all the data is initialized.
    bool mInitialized;

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


    /// Program for displaying the wind for debugging purposes.
    WindQuadGLProgram mWindQuadProgram;
    QOpenGLBuffer *mWindQuadBuffer;
    QOpenGLVertexArrayObject *mWindQuadVAO;


    /* Grass simulation variables for the OpenCL side. */
    cl_mem mGrassWindPositions;         /// "offset" of each grass blade (used for wind effect)
    cl_mem mGrassPeriodOffsets;         /// "velocity" of each grass blade (used for wind effect)
    cl_mem mGrassNormalizedPositions;   /// position of each grass blade, with each coordinate in (0,1)


    /* Wind simulation variables. */
    Fluid2DSimulation *mWindSimulation;

    QOpenGLTexture *mWindVelocities;

    MyCLImage2D mForces1;
    MyCLImage2D mForces2;

    MyCLImage2D *mCurForce;
    MyCLImage2D *mNextForce;

    /* Camera variables. */
    QVector3D mCameraOffset;
    float mCameraPitch;
    float mCameraYaw;
    float mCameraZoom;
    float mCameraZoomSensitivity;
    float mCameraMoveSensitivity;

    /* Variables for dragging. */
    QPoint mDragStart;
    bool mDragging = false;
    QVector3D mDragOffsetStart;
    float mDragPitchStart;
    float mDragYawStart;

    /// Used to keep track of time.
    QTime mApplicationStartTime;

    QTime mLastFrameStartTime;
    QTime mCurrentFrameStartTime;
};

#endif // MAINWINDOW_H
