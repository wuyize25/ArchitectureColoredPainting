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
	if (modelProgramPtr != nullptr)
	{
		makeCurrent();
		delete modelProgramPtr;
		modelProgramPtr = nullptr;
		doneCurrent();
	}
	if (paintingProgramPtr != nullptr)
	{
		makeCurrent();
		delete paintingProgramPtr;
		paintingProgramPtr = nullptr;
		doneCurrent();
	}
	if (finalProgramPtr != nullptr)
	{
		makeCurrent();
		delete finalProgramPtr;
		finalProgramPtr = nullptr;
		doneCurrent();
	}
}


void RendererWidget::initializeGL()
{

	initializeOpenGLFunctions();
	glEnable(GL_DEPTH_TEST);
	glClearColor(0, 0, 0, 1);

	modelProgramPtr = new QOpenGLShaderProgram;
	if (!modelProgramPtr->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/model.vert"))
		qDebug() << "ERROR:" << modelProgramPtr->log();
	if (!modelProgramPtr->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/model.frag"))
		qDebug() << "ERROR:" << modelProgramPtr->log();
	if (!modelProgramPtr->link())
		qDebug() << "ERROR:" << modelProgramPtr->log();

	paintingProgramPtr = new QOpenGLShaderProgram;
	if (!paintingProgramPtr->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/painting.vert"))
		qDebug() << "ERROR:" << paintingProgramPtr->log();
	if (!paintingProgramPtr->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/painting.frag"))
		qDebug() << "ERROR:" << paintingProgramPtr->log();
	if (!paintingProgramPtr->link())
		qDebug() << "ERROR:" << paintingProgramPtr->log();

	finalProgramPtr = new QOpenGLShaderProgram;
	if (!finalProgramPtr->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/final.vert"))
		qDebug() << "ERROR:" << finalProgramPtr->log();
	if (!finalProgramPtr->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Shaders/final.frag"))
		qDebug() << "ERROR:" << finalProgramPtr->log();
	if (!finalProgramPtr->link())
		qDebug() << "ERROR:" << finalProgramPtr->log();

	finalProgramPtr->bind();
	finalProgramPtr->setUniformValue("gBaseColor", 0);
	finalProgramPtr->setUniformValue("gNormal", 1);
	finalProgramPtr->setUniformValue("gPosition", 2);
	finalProgramPtr->setUniformValue("gMetallicRoughness", 3);
	finalProgramPtr->release();

	model = new Model("Models/Sponza/Sponza.gltf", context(), modelProgramPtr, paintingProgramPtr);

	quadVAO.create();
	QOpenGLVertexArrayObject::Binder vaoBinder(&quadVAO);

	quadVBO.create();
	quadVBO.bind();
	GLfloat vertex[] = {
		// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};

	quadVBO.allocate(vertex, sizeof(vertex));
	quadVBO.bind();
	QOpenGLFunctions_4_5_Compatibility* f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_5_Compatibility>();
	f->glEnableVertexAttribArray(0);
	f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
		nullptr);
	f->glEnableVertexAttribArray(1);
	f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
		reinterpret_cast<void*>(3 * sizeof(GLfloat)));
	quadVBO.release();



}
QVector3D lightPositions[] = { QVector3D(0,0,0), QVector3D(100,100,100) ,QVector3D(-100,100,100) ,QVector3D(100,100,-100) };
QVector3D lightColors[] = { QVector3D(150000,150000,130000), QVector3D(0,0,0) ,QVector3D(0,0,0) ,QVector3D(0,0,0) };
void RendererWidget::paintGL()
{
	if (fboPtr->bind())
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		QMatrix4x4 view = camera.GetViewMatrix();
		modelProgramPtr->bind();
		modelProgramPtr->setUniformValue("view", view);
		modelProgramPtr->release();
		paintingProgramPtr->bind();
		paintingProgramPtr->setUniformValue("view", view);
		paintingProgramPtr->release();
		model->draw();
		fboPtr->release();
	}


	//QOpenGLFramebufferObject::blitFramebuffer(nullptr, QRect(0, 0, 2*width(), 2*height()), fboPtr, QRect(0, 0, 1156, 756));
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	QOpenGLVertexArrayObject::Binder vaoBinder(&quadVAO);
	finalProgramPtr->bind();

	finalProgramPtr->setUniformValue("camPos", camera.Position);
	lightPositions[0] = camera.Position;
	finalProgramPtr->setUniformValueArray("lightPositions", lightPositions, 4);
	finalProgramPtr->setUniformValueArray("lightColors", lightColors, 4);

	QVector<GLuint> gbuffers = fboPtr->textures();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gbuffers[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gbuffers[1]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gbuffers[2]);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, gbuffers[3]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	finalProgramPtr->release();
}

void RendererWidget::resizeGL(int width, int height)
{
	//qDebug() << devicePixelRatio() << width << height;
	//glViewport(0, 0, (GLint)devicePixelRatio()*width, (GLint)devicePixelRatio()*height);
	if (fboPtr != nullptr)
		delete fboPtr;
	fboPtr = new QOpenGLFramebufferObject(devicePixelRatio() * width, devicePixelRatio() * height, QOpenGLFramebufferObject::Depth, GL_TEXTURE_2D);
	fboPtr->bind();
	fboPtr->addColorAttachment(devicePixelRatio() * width, devicePixelRatio() * height, GL_RGB16F);
	fboPtr->addColorAttachment(devicePixelRatio() * width, devicePixelRatio() * height, GL_RGB16F);
	fboPtr->addColorAttachment(devicePixelRatio() * width, devicePixelRatio() * height, GL_RG);
	GLenum attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);
	if (fboPtr->bind())
	{
		QMatrix4x4 projection;
		projection.perspective(camera.Zoom, (float)width / (float)height, 10.f, 10000.0f);
		modelProgramPtr->bind();
		modelProgramPtr->setUniformValue("projection", projection);
		modelProgramPtr->release();
		paintingProgramPtr->bind();
		paintingProgramPtr->setUniformValue("projection", projection);
		paintingProgramPtr->release();
		fboPtr->release();
	}
	
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

