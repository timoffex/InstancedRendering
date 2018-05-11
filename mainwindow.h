#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QOpenGLWindow>
#include <QOpenGLExtraFunctions>

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>

#include <QOpenGLShaderProgram>

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

    void updateGrassModel(float bendAngle);
    void createGrassModel(float bendAngle);
    void createGrassOffsets();
    void createGrassVAO();

    /// Stores the data for a single grass blade.
    QOpenGLBuffer *mGrassBladeModelBuffer;

    /// The per-instance data for grass blades.
    QOpenGLBuffer *mGrassBladeInstancedBuffer;

    /// The VAO holding all grass data.
    QOpenGLVertexArrayObject *mGrassVAO;

    /// The grass texture.
    QOpenGLTexture *mGrassTexture;


    /// The shader program that should be used for rendering grass.
    QOpenGLShaderProgram *mGrassProgram;
    int mGrassProgram_vPosition;
    int mGrassProgram_vTexCoord;
    int mGrassProgram_vOffset;
    int mGrassProgram_vRotation;
    int mGrassProgram_uMVP;
    int mGrassProgram_uGrassTexture;

    /// The number of grass blade instances that should be drawn.
    int mNumBlades;

    /// The number of vertices in the grass blade model.
    int mNumVerticesPerBlade;

    /// Variable used for rotating the screen.
    float mRotation = 0;
    QPoint mDragStart;
    float mDragRotationStart;
    bool mDragging = false;

    /// Bend angle for the grass.
    float mGrassBend;
    QTime mStartTime;
};

#endif // MAINWINDOW_H
