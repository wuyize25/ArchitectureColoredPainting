#include "Model.h"

#include <QOpenGLTexture>
#include <QOpenGLContext>
#include <QTextCodec>
#include <iostream>
#include "PaintingMesh.h"
#include <QtMath>

Model::Model(QString path, QOpenGLContext* context, QOpenGLShaderProgram* shaderProgram)
	: context(context)
	, glFunc(context->versionFunctions<QOpenGLFunctions_4_5_Compatibility>())
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
	qDebug() << directory.absolutePath() << "Loaded Successfully";
	qDebug() << "NumMeshes: " << scene->mNumMeshes;
	qDebug() << "NumMaterials: " << scene->mNumMaterials;
	qDebug() << "NumTextures: " << scene->mNumTextures;
	directory.cdUp();
	processNode(scene->mRootNode, scene);
}

Model::Model(QString path, QOpenGLContext* context, QOpenGLShaderProgram* shaderProgram, QOpenGLShaderProgram* paintingProgram)
	: context(context)
	, glFunc(context->versionFunctions<QOpenGLFunctions_4_5_Compatibility>())
	, shaderProgram(shaderProgram)
	, paintingProgram(paintingProgram)
	, directory(path)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(directory.absolutePath().toUtf8(), aiProcess_Triangulate | aiProcess_FlipUVs);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		qDebug() << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
		return;
	}
	qDebug() << directory.absolutePath() << "Loaded Successfully";
	qDebug() << "NumMeshes: " << scene->mNumMeshes;
	qDebug() << "NumMaterials: " << scene->mNumMaterials;
	qDebug() << "NumTextures: " << scene->mNumTextures;
	directory.cdUp();
	processNode(scene->mRootNode, scene);
}

Model::~Model() //销毁对象
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
	//shaderProgram->bind();
	for (Drawable* mesh : meshes) {
		mesh->draw();
	}
}

void Model::destroy()
{
	context->doneCurrent();
	delete this;
}

Model* Model::createModel(QString path, QOpenGLContext* context, QOpenGLShaderProgram* shaderProgram)
{
	return new Model(path, context, shaderProgram);
}


void Model::processNode(aiNode* node, const aiScene* scene, aiMatrix4x4 mat4)
{

	// 处理节点所有的网格（如果有的话）
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene, mat4));

	}
	// 接下来对它的子节点重复这一过程
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene, mat4 * node->mChildren[i]->mTransformation);
	}
}
Drawable* Model::processMesh(aiMesh* mesh, const aiScene* scene, aiMatrix4x4 model)
{
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

	aiString str;
	material->GetTexture(aiTextureType_BASE_COLOR, 0, &str);

	if (paintingProgram != nullptr && std::strcmp(str.C_Str(), "17876391417123941155.jpg") == 0)
	{
		qDebug() << str.C_Str();
		//  初始化网格
		PaintingMesh* m_mesh = new PaintingMesh(glFunc, paintingProgram, model);
		// 遍历网格的每个顶点
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			PaintingVertex vertex;
			vertex.Position = QVector3D(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			// 法向量
			if (mesh->mNormals)
				vertex.Normal = QVector3D(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

			// 纹理坐标
			if (mesh->mTextureCoords[0])
				vertex.TexCoords = QVector2D(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

			if (mesh->mTangents)
				vertex.Tangent = QVector3D(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);

			if (mesh->mBitangents)
				vertex.Bitangent = QVector3D(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);;

			m_mesh->vertices.push_back(vertex);
		}

		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// 将所有面的索引数据添加到索引数组中
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				m_mesh->indices.push_back(face.mIndices[j]);
			}
		}


		GLuint bvhChildren[] = {7/*rootBVH长度*/,0/*与显存对齐*/, 
			//root
			1,2, 
			3,4, 5,6,
			7,0, 7,30./360* 4294967296 /*右儿子用来表示旋转角度*/, 8,0, 7,0,
			//elememt0
			1,2,
			5+28/*contour索引，由于contour不定长，这里需要给到contour在elementIndex中位置*/,5+12/*style索引，在elementData中位置*/, 3,4,
					   5+36,5+12, 5+32,5+12,
			//elememt1
			1+0/*line索引，element中第几条*/,1 + 25

		};
		QVector4D bvhBound[] = { 
			//root
			QVector4D(-1,-1,1,1),
			QVector4D(-0.9,-0.9,-0.1,0.9),  QVector4D(0.1, -0.9,0.9,0.9), 
			QVector4D(-0.8,-0.8,-0.2,-0.1),  QVector4D(-0.7,0.2,-0.2,0.7), QVector4D(0.2,-0.8,0.8,-0.1), QVector4D(0.2,0.1,0.8,0.8),
			//elememt0
			QVector4D(-1,-1,1,1),
			QVector4D(-1,-0.5,1,1),	QVector4D(-1,-1,1,0.5),
									QVector4D(-1,-1,1,-0.5), QVector4D(-1,-0.5,1,0.5),
			//elememt1
			QVector4D(-1,0,1,1),
		};
		glFunc->glGenBuffers(1, &m_mesh->bvhSSBO);
		glFunc->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_mesh->bvhSSBO);
		glFunc->glBufferData(GL_SHADER_STORAGE_BUFFER,sizeof(bvhChildren), bvhChildren, GL_DYNAMIC_DRAW);
		glFunc->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	
		glFunc->glGenBuffers(1, &m_mesh->bvhBoundSSBO);
		glFunc->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_mesh->bvhBoundSSBO);
		glFunc->glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(bvhBound), bvhBound, GL_DYNAMIC_DRAW);
		glFunc->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		GLuint elementOffset[] = {
			//element0
			7, //elementBvhRoot
			5, //elementBvhLength
			0, //pointsOffset
			0, //linesOffset
			//element1
			12, //elementBvhRoot
			1, //elementBvhLength
			19, //pointsOffset
			40, //linesOffset
		};
		glFunc->glGenBuffers(1, &m_mesh->elementOffsetSSBO);
		glFunc->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_mesh->elementOffsetSSBO);
		glFunc->glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(elementOffset), elementOffset, GL_DYNAMIC_DRAW);
		glFunc->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		GLuint elementIndex[] = {
			//element0
			//lines, 全部当作三阶贝塞尔, 每条线四个点索引
			4,2,2,0,
			0,0,1,1,
			1,1,4,4,
			1,1,5,5,
			4,4,5,5,
			1,1,3,3,
			3,3,5,5,
			//contours, 第一个元素指明轮廓段数，后面为lines索引
			3, 0,1,2,
			3, 2,3,4,
			3, 3,5,6,

			//element2
			//lines
			0,1,2
		};
		glFunc->glGenBuffers(1, &m_mesh->elementIndexSSBO);
		glFunc->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_mesh->elementIndexSSBO);
		glFunc->glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(elementIndex), elementIndex, GL_DYNAMIC_DRAW);
		glFunc->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


		GLfloat elementData[] = {
			//element0
			//points
			-1,0.5, -1,-0.5, 0,1, 0,-1, 1,0.5, 1,-0.5,
			//fillStyle
			//fill
			0, 
			//fillType
			0, //单色
			//fillColorMetallicRoughness
			1,1,0, 0,0.8,

			//element1
			//points
			-1,0.5, 0,1, 1,0.5,
			//strokeStyle
			//stroke
			1,
			//strokeWidth
			0.02,
			//strokeEndType
			0, //圆角
			//strokeFillType
			0, //单色
			//strokeFillColorMetallicRoughness
			0,1,0, 0,0.8
		};
		glFunc->glGenBuffers(1, &m_mesh->elementDataSSBO);
		glFunc->glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_mesh->elementDataSSBO);
		glFunc->glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(elementData), elementData, GL_DYNAMIC_DRAW);
		glFunc->glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


		m_mesh->setupMesh();
		return m_mesh;
	}
	else
	{
		//  初始化网格
		Mesh* m_mesh = new Mesh(glFunc, shaderProgram, model);
		// 遍历网格的每个顶点
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			QVector3D vector; //将assimp的数据转化为QtOpenGL支持的数据

			// 位置
			vector.setX(mesh->mVertices[i].x);
			vector.setY(mesh->mVertices[i].y);
			vector.setZ(mesh->mVertices[i].z);
			vertex.Position = vector;
			// 法向量
			if (mesh->mNormals) {
				vector.setX(mesh->mNormals[i].x);
				vector.setY(mesh->mNormals[i].y);
				vector.setZ(mesh->mNormals[i].z);
				vertex.Normal = vector;
			}
			// 纹理坐标
			if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
			{
				QVector2D vec;
				//一个顶点最多可以包含8个不同的纹理坐标。因此我们假设我们不用
				//使用一个顶点可以有多个纹理坐标的模型，所以我们总是取第一个集合(0)。
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
			// 将所有面的索引数据添加到索引数组中
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				m_mesh->indices.push_back(face.mIndices[j]);
			}
		}

		// 处理材质
		QVector<Texture*> diffuseMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_basecolor");
		for (auto& it : diffuseMaps)
			m_mesh->textures.push_back(it);

		QVector<Texture*> metalnessMaps = loadMaterialTextures(material, aiTextureType_METALNESS, "texture_metallic_roughness");
		for (auto& it : metalnessMaps)
			m_mesh->textures.push_back(it);

		QVector<Texture*> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal");
		for (auto& it : normalMaps)
			m_mesh->textures.push_back(it);

		m_mesh->setupMesh();
		return m_mesh;
	}


}

QVector<Texture*> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, QString typeName)
{
	QVector<Texture*> textures;

	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		//qDebug() << typeName;
		aiString str;
		mat->GetTexture(type, i, &str);

		// 检查纹理是否在之前加载过，如果是，则继续到下一个迭代:跳过加载新纹理
		bool skip = false;
		for (unsigned int j = 0; j < textures_loaded.size(); j++)
		{
			if (std::strcmp(textures_loaded[j]->path.toStdString().c_str(), str.C_Str()) == 0)
			{
				textures.push_back(textures_loaded[j]);
				skip = true; //【优化】 带有相同filepath的纹理已经加载，继续到下一个
				break;
			}
		}
		if (!skip)
		{   // 如果材质还没有加载，加载它
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
				qDebug() << "未能成功加载纹理：" << directory.filePath(str.C_Str());
			}
		}
	}
	return textures;
}
