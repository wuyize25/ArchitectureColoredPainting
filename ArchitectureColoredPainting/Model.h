#pragma once

#include "Mesh.h"
#include "Drawable.h"
#include <QDir>

class Model
{
public:
	void draw();
	void destroy();
	static Model* createModel(QString path, QOpenGLContext* context, QOpenGLShaderProgram* shaderProgram);
	Model(QString path, QOpenGLContext* context, QOpenGLShaderProgram* shaderProgram, QOpenGLShaderProgram* paintingProgram);
private:
	Model(QString path, QOpenGLContext* context, QOpenGLShaderProgram* shaderProgram);

	~Model();
	QOpenGLContext* context;                //opengl�������
	QOpenGLFunctions_4_5_Compatibility* glFunc;
	QOpenGLShaderProgram* shaderProgram = nullptr;
	QOpenGLShaderProgram* paintingProgram = nullptr;  //�ʻ���ɫ������
	/*  ģ������  */
	QVector<Texture*> textures_loaded;      //����
	QVector<Drawable*> meshes;                  //����
	QDir directory;                         //ģ������·��

	//�ݹ�������
	void processNode(aiNode* node, const aiScene* scene, aiMatrix4x4 mat4 = aiMatrix4x4());

	//��������
	Drawable* processMesh(aiMesh* mesh, const aiScene* scene, aiMatrix4x4 model);

	//���ز�������
	QVector<Texture*> loadMaterialTextures(aiMaterial* mat, aiTextureType type, QString typeName);

};

