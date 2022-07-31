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
	QOpenGLContext* context;                //opengl函数入口
	QOpenGLFunctions_4_5_Compatibility* glFunc;
	QOpenGLShaderProgram* shaderProgram = nullptr;
	QOpenGLShaderProgram* paintingProgram = nullptr;  //彩绘着色器程序
	/*  模型数据  */
	QVector<Texture*> textures_loaded;      //纹理
	QVector<Drawable*> meshes;                  //网格
	QDir directory;                         //模型所在路径

	//递归遍历结点
	void processNode(aiNode* node, const aiScene* scene, aiMatrix4x4 mat4 = aiMatrix4x4());

	//加载网格
	Drawable* processMesh(aiMesh* mesh, const aiScene* scene, aiMatrix4x4 model);

	//加载材质纹理
	QVector<Texture*> loadMaterialTextures(aiMaterial* mat, aiTextureType type, QString typeName);

};

