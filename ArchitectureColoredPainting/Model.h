#pragma once

#include "Mesh.h"
#include <QDir>

class Model
{
public:
    void draw();
    void destroy();
    static Model* createModel(QString path, QOpenGLContext* context, QOpenGLShaderProgram* shaderProgram);

private:
    Model(QString path, QOpenGLContext* context, QOpenGLShaderProgram* shaderProgram);
    ~Model();
    QOpenGLContext* context;          //opengl�������
    QOpenGLShaderProgram* shaderProgram;   //��ɫ������

    /*  ģ������  */
    QVector<Texture*>textures_loaded;       //����
    QVector<Mesh*> meshes;                  //����
    QDir directory;                         //ģ������·��

    //�ݹ�������
    void processNode(aiNode* node, const aiScene* scene, aiMatrix4x4 mat4 = aiMatrix4x4());

    //��������
    Mesh* processMesh(aiMesh* mesh, const aiScene* scene, aiMatrix4x4 model);

    //���ز�������
    QVector<Texture*> loadMaterialTextures(aiMaterial* mat, aiTextureType type, QString typeName);

};

