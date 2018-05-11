#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QOpenGLWindow>
#include <QOpenGLExtraFunctions>

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>

#include <QOpenGLShaderProgram>

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


    bool checkGLErrors();

private:

    void createGrassModel();
    void createGrassOffsets();
    void createGrassVAO();

    /// Stores the data for a single grass blade.
    QOpenGLBuffer *mGrassBladeModelBuffer;

    /// The per-instance data for grass blades.
    QOpenGLBuffer *mGrassBladeOffsetsInstancedBuffer;

    /// The VAO holding all grass data.
    QOpenGLVertexArrayObject *mGrassVAO;

    /// The grass texture.
    QOpenGLTexture *mGrassTexture;


    /// The shader program that should be used for rendering grass.
    QOpenGLShaderProgram *mGrassProgram;
    int mGrassProgram_vPosition;
    int mGrassProgram_vTexCoord;
    int mGrassProgram_vOffset;
    int mGrassProgram_uMVP;
    int mGrassProgram_uGrassTexture;

    /// The number of grass blade instances that should be drawn.
    int mNumBlades;

    /// The number of vertices in the grass blade model.
    int mNumVerticesPerBlade;


    float mRotation = 0;
};

#endif // MAINWINDOW_H
