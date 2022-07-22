#include "RendererWidget.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

RendererWidget::RendererWidget(QWidget* parent)
	: QOpenGLWidget(parent), camera(QVector3D(0.0f, 100.0f, 0.0f))
{
	startTimer(1000 / 120);
	lastFrame = std::clock();
	setFocusPolicy(Qt::StrongFocus);

}

RendererWidget::~RendererWidget()
{
	if (m_program == nullptr)
		return;
	makeCurrent();
	m_vbo.destroy();
	delete m_program;
	m_program = nullptr;
	doneCurrent();
}


void RendererWidget::initializeGL()
{

	initializeOpenGLFunctions();
	glEnable(GL_DEPTH_TEST);
	glClearColor(0, 0, 0, 1);

	m_program = new QOpenGLShaderProgram;
	if (!m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/shader.vert"))
		qDebug() << "ERROR:" << m_program->log();
	if (!m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/shader.frag"))
		qDebug() << "ERROR:" << m_program->log();
	if (!m_program->link())
		qDebug() << "ERROR:" << m_program->log();
	m_program->bind();

	model = Model::createModel("Models/Sponza/Sponza.gltf", context(), m_program);


	/*m_vao.create();
	QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);

	m_vbo.create();
	m_vbo.bind();


	GLfloat vertex[] = {
		-1,-1, 0, 0,0,
		 1,-1, 0, 1,0,
		-1, 1, 0, 0,1,
	};

	m_vbo.allocate(vertex, sizeof(vertex));
	m_vbo.bind();
	QOpenGLFunctions_4_5_Compatibility* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_5_Compatibility>();
	f->glEnableVertexAttribArray(0);
	f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
		nullptr);
	f->glEnableVertexAttribArray(1);
	f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
		reinterpret_cast<void*>(3 * sizeof(GLfloat)));
	m_vbo.release();*/
}
QVector3D lightPositions[] = { QVector3D(0,0,0), QVector3D(100,100,100) ,QVector3D(-100,100,100) ,QVector3D(100,100,-100) };
QVector3D lightColors[] = { QVector3D(150000,150000,150000), QVector3D(0,0,0) ,QVector3D(0,0,0) ,QVector3D(0,0,0) };
void RendererWidget::paintGL()
{
	//std::cout << (double)CLOCKS_PER_SEC/(std::clock() -lastFrame) << std::endl;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	QOpenGLVertexArrayObject::Binder vaoBinder(&m_vao);
	m_program->bind();
	// camera/view transformation
	QMatrix4x4 view = camera.GetViewMatrix();
	m_program->setUniformValue(m_program->uniformLocation("view"), view);
	m_program->setUniformValue(m_program->uniformLocation("camPos"), camera.Position);
	lightPositions[0] = camera.Position;
	m_program->setUniformValueArray("lightPositions", lightPositions, 4);
	m_program->setUniformValueArray("lightColors", lightColors, 4);
	//glDrawArrays(GL_TRIANGLES, 0, 3);
	model->draw();
	m_program->release();
}

void RendererWidget::resizeGL(int width, int height)
{
	m_program->bind();
	QMatrix4x4 projection;
	projection.perspective(camera.Zoom, (float)width / (float)height, 10.f, 10000.0f);
	m_program->setUniformValue(m_program->uniformLocation("projection"), projection);

}

void RendererWidget::timerEvent(QTimerEvent* event)
{
	clock_t currentFrame = std::clock();
	deltaTime = (float)(std::clock() - lastFrame) / CLOCKS_PER_SEC;
	lastFrame = currentFrame;

	if (hasFocus())
	{
		QPoint center = mapToGlobal(geometry().center());
		float xoffset = cursor().pos().x() - center.x();
		float yoffset = center.y() - cursor().pos().y();
		camera.ProcessMouseMovement(xoffset, yoffset);
		cursor().setPos(center);
	}

	if (pressedKeys.contains(Qt::Key_W)) {
		camera.ProcessKeyboard(FORWARD, deltaTime);
	}
	if (pressedKeys.contains(Qt::Key_S)) {
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	}
	if (pressedKeys.contains(Qt::Key_A)) {
		camera.ProcessKeyboard(LEFT, deltaTime);
	}
	if (pressedKeys.contains(Qt::Key_D)) {
		camera.ProcessKeyboard(RIGHT, deltaTime);
	}
	if (pressedKeys.contains(Qt::Key_Shift)) {
		camera.ProcessKeyboard(DOWN, deltaTime);
	}
	if (pressedKeys.contains(Qt::Key_Space)) {
		camera.ProcessKeyboard(UP, deltaTime);
	}
	repaint();
}

void RendererWidget::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape)
		clearFocus();
	else if (!event->isAutoRepeat())
		pressedKeys.insert(event->key());
	QOpenGLWidget::keyPressEvent(event);
}

void RendererWidget::keyReleaseEvent(QKeyEvent* event)
{
	if (!event->isAutoRepeat())
		pressedKeys.remove(event->key());
	QOpenGLWidget::keyReleaseEvent(event);
}


void RendererWidget::focusInEvent(QFocusEvent* event)
{
	setCursor(Qt::BlankCursor);
	cursor().setPos(mapToGlobal(geometry().center()));
}

void RendererWidget::focusOutEvent(QFocusEvent* event)
{
	setCursor(Qt::ArrowCursor);
}

