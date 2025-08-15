#include "ResourceManager.h"

#include <ECS/Global.h>

#include <iostream>

namespace ECS
{
    ResourceManager::ResourceManager()
    {
        // TODO: Implement
        std::cout << "ResourceManager created\n";
    }

    ResourceManager::~ResourceManager()
    {
        // TODO: Implement
        std::cout << "ResourceManager destroyed\n";
    }

    entt::entity ResourceManager::LoadModel(const std::string &filePath)
    {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return entt::null;
        }

        auto modelEntity = ECS::Global::get().registry->create();
        ECS::Global::get().registry->emplace<Components::Meshes>(modelEntity, Components::Meshes{
                                                                                  .meshes = {},
                                                                                  .path = filePath});

        ProcessNode(scene->mRootNode, scene, modelEntity);

        return modelEntity;
    }

    void ResourceManager::ProcessNode(aiNode *node, const aiScene *scene, entt::entity parentEntity)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            entt::entity meshEntity = ProcessMesh(mesh, scene, parentEntity);
            auto &meshes = ECS::Global::get().registry->get<Components::Meshes>(parentEntity);
            meshes.meshes.push_back(meshEntity);
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            ProcessNode(node->mChildren[i], scene, parentEntity);
        }
    }

    entt::entity ResourceManager::ProcessMesh(aiMesh *mesh, const aiScene *scene, entt::entity modelEntity)
    {
        auto meshEntity = ECS::Global::get().registry->create();

        Components::MeshHost meshHost;

        meshHost.positions.reserve(mesh->mNumVertices * 3);
        meshHost.normals.reserve(mesh->mNumVertices * 3);
        meshHost.texCoords.reserve(mesh->mNumVertices * 2);
        meshHost.boneIndices.resize(mesh->mNumVertices * 4, 0);
        meshHost.boneWeights.resize(mesh->mNumVertices * 4, 0.0f);

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            meshHost.positions.push_back(mesh->mVertices[i].x);
            meshHost.positions.push_back(mesh->mVertices[i].y);
            meshHost.positions.push_back(mesh->mVertices[i].z);

            if (mesh->HasNormals())
            {
                meshHost.normals.push_back(mesh->mNormals[i].x);
                meshHost.normals.push_back(mesh->mNormals[i].y);
                meshHost.normals.push_back(mesh->mNormals[i].z);
            }

            if (mesh->HasTextureCoords(0))
            {
                meshHost.texCoords.push_back(mesh->mTextureCoords[0][i].x);
                meshHost.texCoords.push_back(mesh->mTextureCoords[0][i].y);
            }
            else
            {
                meshHost.texCoords.push_back(0.0f);
                meshHost.texCoords.push_back(0.0f);
            }

            meshHost.boneIndices.push_back(0);
            meshHost.boneIndices.push_back(0);
            meshHost.boneIndices.push_back(0);
            meshHost.boneIndices.push_back(0);

            meshHost.boneWeights.push_back(0.0f);
            meshHost.boneWeights.push_back(0.0f);
            meshHost.boneWeights.push_back(0.0f);
            meshHost.boneWeights.push_back(0.0f);
        }

        ExtractBoneWeightForVertices(mesh, meshHost, scene, modelEntity);

        meshHost.indices.reserve(mesh->mNumFaces * 3);
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
            {
                meshHost.indices.push_back(face.mIndices[j]);
            }
        }

        if (mesh->mMaterialIndex >= 0)
        {
        }

        return meshEntity;
    }

    void ResourceManager::ExtractBoneWeightForVertices(aiMesh *mesh, Components::MeshHost &meshHost, const aiScene *scene, entt::entity modelEntity)
    {
        auto &animations = ECS::Global::get().registry->get<Components::Animations>(modelEntity);
        auto &boneInfoMap = animations.boneInfoMap;
        int &boneCount = animations.boneCount;

        auto ConvertMatrixToKTFormat = [](const aiMatrix4x4 &from) -> ktm::fmat4x4 {
            ktm::fmat4x4 to;
            to[0][0] = from.a1;
            to[1][0] = from.a2;
            to[2][0] = from.a3;
            to[3][0] = from.a4;
            to[0][1] = from.b1;
            to[1][1] = from.b2;
            to[2][1] = from.b3;
            to[3][1] = from.b4;
            to[0][2] = from.c1;
            to[1][2] = from.c2;
            to[2][2] = from.c3;
            to[3][2] = from.c4;
            to[0][3] = from.d1;
            to[1][3] = from.d2;
            to[2][3] = from.d3;
            to[3][3] = from.d4;
            return to;
        };

        for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
        {
            int boneID = -1;
            std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

            if (boneInfoMap.find(boneName) == boneInfoMap.end())
            {
                Components::BoneInfo newBoneInfo;
                newBoneInfo.id = boneCount;
                newBoneInfo.offsetMatrix = ConvertMatrixToKTFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
                boneInfoMap[boneName] = newBoneInfo;
                boneID = boneCount;
                boneCount++;
            }
            else
            {
                boneID = boneInfoMap[boneName].id;
            }

            auto weights = mesh->mBones[boneIndex]->mWeights;
            int numWeights = mesh->mBones[boneIndex]->mNumWeights;

            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;

                for (int i = 0; i < 4; ++i)
                {
                    if ((meshHost.boneIndices[vertexId * 4 + i] == 0) && (meshHost.boneWeights[vertexId * 4 + i] <= 1e-3))
                    {
                        meshHost.boneWeights[vertexId * 4 + i] = weight;
                        meshHost.boneIndices[vertexId * 4 + i] = boneID;
                        break;
                    }
                }
            }
        }
    }

} // namespace ECS