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
	GLuint padding[2];//与显存对齐
	QVector4D bound;
	BvhNode(GLuint leftChild, GLuint rightChild, QVector4D bound) :leftChild(leftChild), rightChild(rightChild), bound(bound) {}
};

class PaintingMesh : public Drawable
{
public:
	/*  网格数据  */
	QVector<PaintingVertex> vertices;               //顶点数据
	QVector<unsigned int> indices;          //索引数组
	QMatrix4x4 model;                       //模型矩阵
	QOpenGLFunctions_4_5_Compatibility* glFunc;               //opengl函数入口
	QOpenGLShaderProgram* shaderProgram;    //着色器程序

	GLuint bvhSSBO, bvhBoundSSBO, elementIndexSSBO, elementSSBO;
	
	/*  函数  */
	PaintingMesh(QOpenGLFunctions_4_5_Compatibility* glFunc, QOpenGLShaderProgram* shaderProgram, aiMatrix4x4 model);
	void draw() override;
	void setupMesh();

private:
	/*  渲染数据  */
	QOpenGLVertexArrayObject VAO;
	QOpenGLBuffer VBO, EBO;

};
