#pragma once

#include <QOpenGLFunctions_4_5_Compatibility>
#include <QString>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Drawable.h"

struct PaintingVertex
{
	QVector3D Position;
	QVector3D Normal;
	QVector2D TexCoords;
	QVector3D Tangent;
	QVector3D Bitangent;
};

struct BvhNode {
	GLuint leftChild;
	GLuint rightChild;
	GLuint padding[2];//���Դ����
	QVector4D bound;
	BvhNode(GLuint leftChild, GLuint rightChild, QVector4D bound) :leftChild(leftChild), rightChild(rightChild), bound(bound) {}
};

class PaintingMesh : public Drawable
{
public:
	/*  ��������  */
	QVector<PaintingVertex> vertices;               //��������
	QVector<unsigned int> indices;          //��������
	QMatrix4x4 model;                       //ģ�;���
	QOpenGLFunctions_4_5_Compatibility* glFunc;               //opengl�������
	QOpenGLShaderProgram* shaderProgram;    //��ɫ������

	GLuint bvhSSBO, bvhBoundSSBO, elementIndexSSBO, elementSSBO;
	
	/*  ����  */
	PaintingMesh(QOpenGLFunctions_4_5_Compatibility* glFunc, QOpenGLShaderProgram* shaderProgram, aiMatrix4x4 model);
	void draw() override;
	void setupMesh();

private:
	/*  ��Ⱦ����  */
	QOpenGLVertexArrayObject VAO;
	QOpenGLBuffer VBO, EBO;

};
