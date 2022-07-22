#include "Mesh.h"

Mesh::Mesh(QOpenGLFunctions_4_5_Compatibility* glFunc, QOpenGLShaderProgram* shaderProgram, aiMatrix4x4 model)
    : glFunc(glFunc)
    , shaderProgram(shaderProgram)
    , VBO(QOpenGLBuffer::VertexBuffer)
    , EBO(QOpenGLBuffer::IndexBuffer)
{

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            this->model(i, j) = model[i][j];
        }
    }
}
void Mesh::Draw()
{
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr = 1;
    unsigned int heightNr = 1;

    for (unsigned int i = 0; i < textures.size(); i++)
    {
        glFunc->glActiveTexture(GL_TEXTURE0 + i); // 在绑定之前激活相应的纹理单元
     
      
        textures[i]->texture.bind();
        //qDebug() << name + number;
        shaderProgram->setUniformValue(textures[i]->type.toStdString().c_str(), i);
    }
    // 绘制网格

    QOpenGLVertexArrayObject::Binder bind(&VAO);
    shaderProgram->setUniformValue("model", model);
    glFunc->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}
void Mesh::setupMesh()
{

    VAO.create();
    VAO.bind();

    VBO.create();
    EBO.create();

    VBO.bind();
    VBO.allocate(vertices.data(), vertices.size() * sizeof(Vertex));

    EBO.bind();
    EBO.allocate(indices.data(), indices.size() * sizeof(unsigned int));


    shaderProgram->enableAttributeArray(0);
    shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(Vertex));

    shaderProgram->enableAttributeArray(1);
    shaderProgram->setAttributeBuffer(1, GL_FLOAT, offsetof(Vertex, Normal), 3, sizeof(Vertex));

    shaderProgram->enableAttributeArray(2);
    shaderProgram->setAttributeBuffer(2, GL_FLOAT, offsetof(Vertex, TexCoords), 2, sizeof(Vertex));

    shaderProgram->enableAttributeArray(1);
    shaderProgram->setAttributeBuffer(3, GL_FLOAT, offsetof(Vertex, Tangent), 3, sizeof(Vertex));

    shaderProgram->enableAttributeArray(1);
    shaderProgram->setAttributeBuffer(4, GL_FLOAT, offsetof(Vertex, Bitangent), 3, sizeof(Vertex));
    VAO.release();
}
