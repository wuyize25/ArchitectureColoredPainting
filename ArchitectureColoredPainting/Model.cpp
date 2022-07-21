#include "Model.h"

#include <QOpenGLTexture>
#include <QOpenGLContext>
#include <QTextCodec>
#include <iostream>

Model::Model(QString path, QOpenGLContext* context, QOpenGLShaderProgram* shaderProgram)
    : context(context)
    , shaderProgram(shaderProgram)
    , directory(path)
{
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(directory.absolutePath().toUtf8(), aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        qDebug() << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
        return;
    }
    qDebug() << directory.absolutePath() << "Load Successfully";
    qDebug() << "NumMeshes: " << scene->mNumMeshes;
    qDebug() << "NumMaterials: " << scene->mNumMaterials;
    qDebug() << "NumTextures: " << scene->mNumTextures;
    directory.cdUp();
    processNode(scene->mRootNode, scene);
}

Model::~Model() //���ٶ���
{
    for (auto& it : textures_loaded) {
        it->texture.destroy();
        delete it;
    }
    for (auto& it : meshes) {
        delete it;
    }
}

void Model::draw() {
    shaderProgram->bind();
    for (Mesh* mesh : meshes) {
        mesh->Draw();
    }
}

void Model::destroy()
{
    delete this;
    context->doneCurrent();
}

Model* Model::createModel(QString path, QOpenGLContext* context, QOpenGLShaderProgram* shaderProgram)
{
    return new Model(path, context, shaderProgram);
}


void Model::processNode(aiNode* node, const aiScene* scene, aiMatrix4x4 mat4)
{

    // ����ڵ����е���������еĻ���
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene, mat4));

    }
    // �������������ӽڵ��ظ���һ����
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene, mat4 * node->mChildren[i]->mTransformation);
    }
}
Mesh* Model::processMesh(aiMesh* mesh, const aiScene* scene, aiMatrix4x4 model)
{
    //  ��ʼ������
    Mesh* m_mesh = new Mesh(QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_4_5_Compatibility>(), shaderProgram, model);
    // ���������ÿ������
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        QVector3D vector; //��assimp������ת��ΪQtOpenGL֧�ֵ�����

        // λ��
        vector.setX(mesh->mVertices[i].x);
        vector.setY(mesh->mVertices[i].y);
        vector.setZ(mesh->mVertices[i].z);
        vertex.Position = vector;
        // ������
        if (mesh->mNormals) {
            vector.setX(mesh->mNormals[i].x);
            vector.setY(mesh->mNormals[i].y);
            vector.setZ(mesh->mNormals[i].z);
            vertex.Normal = vector;
        }
        // ��������
        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            QVector2D vec;
            //һ�����������԰���8����ͬ���������ꡣ������Ǽ������ǲ���
            //ʹ��һ����������ж�����������ģ�ͣ�������������ȡ��һ������(0)��
            vec.setX(mesh->mTextureCoords[0][i].x);
            vec.setY(mesh->mTextureCoords[0][i].y);
            vertex.TexCoords = vec;
        }
        else {
            vertex.TexCoords = QVector2D(0, 0);
        }
        if (mesh->mTangents) {
            // tangent
            vector.setX(mesh->mTangents[i].x);
            vector.setY(mesh->mTangents[i].y);
            vector.setZ(mesh->mTangents[i].z);
            vertex.Tangent = vector;
        }
        if (mesh->mBitangents) {
            vector.setX(mesh->mBitangents[i].x);
            vector.setY(mesh->mBitangents[i].y);
            vector.setZ(mesh->mBitangents[i].z);
            vertex.Bitangent = vector;
        }
        // bitangent
        m_mesh->vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // �������������������ӵ�����������
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            m_mesh->indices.push_back(face.mIndices[j]);
        }
    }

    // �������
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    // 1. ��������ͼ
    QVector<Texture*> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    for (auto& it : diffuseMaps)
        m_mesh->textures.push_back(it);

    // 2. ������ͼ
    QVector<Texture*> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    for (auto& it : specularMaps)
        m_mesh->textures.push_back(it);

    // 3. ������ͼ
    QVector<Texture*> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    for (auto& it : normalMaps)
        m_mesh->textures.push_back(it);

    // 4. �߶�ͼ
    QVector<Texture*> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    for (auto& it : heightMaps)
        m_mesh->textures.push_back(it);

    m_mesh->setupMesh();
    return m_mesh;
}

QVector<Texture*> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, QString typeName)
{
    QVector<Texture*> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        // ��������Ƿ���֮ǰ���ع�������ǣ����������һ������:��������������
        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            if (std::strcmp(textures_loaded[j]->path.toStdString().c_str(), str.C_Str()) == 0)
            {
                textures.push_back(textures_loaded[j]);
                skip = true; //���Ż��� ������ͬfilepath�������Ѿ����أ���������һ��
                break;
            }
        }
        if (!skip)
        {   // ������ʻ�û�м��أ�������
            Texture* texture = new Texture;
            QImage data(directory.filePath(str.C_Str()));
            if (!data.isNull()) {
                texture->texture.setData(data);
                texture->type = typeName;
                texture->path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
            else {
                qDebug() << "δ�ܳɹ���������" << directory.filePath(str.C_Str());
            }
        }
    }
    return textures;
}
