#include "PaintingMesh.h"

PaintingMesh::PaintingMesh(QOpenGLFunctions_4_5_Compatibility* glFunc, QOpenGLShaderProgram* shaderProgram, aiMatrix4x4 model)
    : glFunc(glFunc)
    , shaderProgram(shaderProgram)
    , VBO(QOpenGLBuffer::VertexBuffer)
    , EBO(QOpenGLBuffer::IndexBuffer)
    , model((float*)&model)
{
}
void PaintingMesh::draw()
{
   
    shaderProgram->bind();
    QOpenGLVertexArrayObject::Binder bind(&VAO);
    shaderProgram->setUniformValue("model", model);

    glFunc->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bvhSSBO);
    glFunc->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, bvhBoundSSBO);
    glFunc->glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, elementSSBO);

    glFunc->glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glFunc->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    shaderProgram->release();
}
void PaintingMesh::setupMesh()
{
    shaderProgram->bind();
    VAO.create();
    VAO.bind();

    VBO.create();
    EBO.create();

    VBO.bind();
    VBO.allocate(vertices.data(), vertices.size() * sizeof(PaintingVertex));

    EBO.bind();
    EBO.allocate(indices.data(), indices.size() * sizeof(unsigned int));

 
    shaderProgram->enableAttributeArray(0);
    shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(PaintingVertex));

    shaderProgram->enableAttributeArray(1);
    shaderProgram->setAttributeBuffer(1, GL_FLOAT, offsetof(PaintingVertex, Normal), 3, sizeof(PaintingVertex));

    shaderProgram->enableAttributeArray(2);
    shaderProgram->setAttributeBuffer(2, GL_FLOAT, offsetof(PaintingVertex, TexCoords), 2, sizeof(PaintingVertex));

    shaderProgram->enableAttributeArray(1);
    shaderProgram->setAttributeBuffer(3, GL_FLOAT, offsetof(PaintingVertex, Tangent), 3, sizeof(PaintingVertex));

    shaderProgram->enableAttributeArray(1);
    shaderProgram->setAttributeBuffer(4, GL_FLOAT, offsetof(PaintingVertex, Bitangent), 3, sizeof(PaintingVertex)); 
    

    VAO.release();

    shaderProgram->release();
    
}
