#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Compatibility>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QKeyEvent>
#include "Camera.h"
#include "Model.h"

class RendererWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Compatibility
{
    Q_OBJECT
public:
    RendererWidget(QWidget* parent = nullptr);
    ~RendererWidget();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void timerEvent(QTimerEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
private:
    QSet<int> pressedKeys;
    Camera camera;
    clock_t lastFrame;
    float deltaTime;
    QOpenGLShaderProgram* m_program = nullptr;
    QOpenGLBuffer m_vbo;
    QOpenGLVertexArrayObject m_vao;
    Model* model;
};

