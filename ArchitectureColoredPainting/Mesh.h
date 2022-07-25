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

struct Vertex
{
    QVector3D Position;
    QVector3D Normal;
    QVector2D TexCoords;
    QVector3D Tangent;
    QVector3D Bitangent;
};

struct Texture
{
    QOpenGLTexture texture;
    QString type;
    QString path;
    Texture() :texture(QOpenGLTexture::Target2D) {
        texture.create();
        texture.setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::Repeat);
        texture.setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);
        texture.setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
    }
};

class Mesh : public Drawable
{
public:
    /*  网格数据  */
    QVector<Vertex> vertices;               //顶点数据
    QVector<unsigned int> indices;          //索引数组
    QVector<Texture*> textures;             //纹理数据
    QMatrix4x4 model;                       //模型矩阵
    QOpenGLFunctions_4_5_Compatibility* glFunc;               //opengl函数入口
    QOpenGLShaderProgram* shaderProgram;    //着色器程序

    /*  函数  */
    Mesh(QOpenGLFunctions_4_5_Compatibility* glFunc, QOpenGLShaderProgram* shaderProgram, aiMatrix4x4 model);
    void draw() override;
    void setupMesh();

private:
    /*  渲染数据  */
    QOpenGLVertexArrayObject VAO;
    QOpenGLBuffer VBO, EBO;

};
