#include "ResourceManager.h"

#include <ECS/Global.h>

#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
            std::printf("ERROR::ASSIMP::%s\n", importer.GetErrorString());
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

    std::string ResourceManager::loadShader(const std::string &shaderPath)
    {
        std::ifstream file(shaderPath.data());
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open the file.");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        file.close();
        return buffer.str();
    }

    void ResourceManager::loadDemo(const std::string &demoPath, entt::entity modelEntity)
    {
        std::string imagePath = demoPath+"/awesomeface.png";
        int width, height, channels;
        unsigned char *data = stbi_load(imagePath.c_str(), &width, &height, &channels, 0);
        ECS::Global::get().registry->emplace<Components::ImageHost>(modelEntity, Components::ImageHost{
                                                                                  .path = imagePath,
                                                                                  .data = data,
                                                                                  .width = width,
                                                                                  .height = height,
                                                                                  .channels = channels});
        
        ECS::Global::get().registry->emplace<Components::ImageDevice>(modelEntity, Components::ImageDevice{
                                                                                  .image = HardwareImage(ktm::uvec2(width, height), ImageFormat::RGBA8_SRGB, ImageUsage::SampledImage, 1, data)});

        createMesh(modelEntity);

        RasterizerPipeline rasterizerPipeline(loadShader(demoPath + "/vert.glsl"), loadShader(demoPath + "/frag.glsl"));
        ComputePipeline computePipeline(loadShader(demoPath + "/compute.glsl"));

        ECS::Global::get().registry->emplace<Components::Pipeline>(modelEntity, Components::Pipeline{
            .rasterizerPipeline = rasterizerPipeline,
            .computePipeline = computePipeline
        });

        ECS::Global::get().registry->emplace<Components::ResLoadedTag>(modelEntity);
    }


    void ResourceManager::createMesh(entt::entity modelEntity)
    {
        std::vector<float> pos = {
            -0.5f,
            -0.5f,
            -0.5f,
            0.5f,
            -0.5f,
            -0.5f,
            0.5f,
            0.5f,
            -0.5f,
            0.5f,
            0.5f,
            -0.5f,
            -0.5f,
            0.5f,
            -0.5f,
            -0.5f,
            -0.5f,
            -0.5f,

            -0.5f,
            -0.5f,
            0.5f,
            0.5f,
            -0.5f,
            0.5f,
            0.5f,
            0.5f,
            0.5f,
            0.5f,
            0.5f,
            0.5f,
            -0.5f,
            0.5f,
            0.5f,
            -0.5f,
            -0.5f,
            0.5f,

            -0.5f,
            0.5f,
            0.5f,
            -0.5f,
            0.5f,
            -0.5f,
            -0.5f,
            -0.5f,
            -0.5f,
            -0.5f,
            -0.5f,
            -0.5f,
            -0.5f,
            -0.5f,
            0.5f,
            -0.5f,
            0.5f,
            0.5f,

            0.5f,
            0.5f,
            0.5f,
            0.5f,
            0.5f,
            -0.5f,
            0.5f,
            -0.5f,
            -0.5f,
            0.5f,
            -0.5f,
            -0.5f,
            0.5f,
            -0.5f,
            0.5f,
            0.5f,
            0.5f,
            0.5f,

            -0.5f,
            -0.5f,
            -0.5f,
            0.5f,
            -0.5f,
            -0.5f,
            0.5f,
            -0.5f,
            0.5f,
            0.5f,
            -0.5f,
            0.5f,
            -0.5f,
            -0.5f,
            0.5f,
            -0.5f,
            -0.5f,
            -0.5f,

            -0.5f,
            0.5f,
            -0.5f,
            0.5f,
            0.5f,
            -0.5f,
            0.5f,
            0.5f,
            0.5f,
            0.5f,
            0.5f,
            0.5f,
            -0.5f,
            0.5f,
            0.5f,
            -0.5f,
            0.5f,
            -0.5f,
        };

        std::vector<float> normal = {
            // Back face (Z-negative)
            0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, // Triangle 1
            0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, // Triangle 2

            // Front face (Z-positive)
            0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // Triangle 1
            0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // Triangle 2

            // Left face (X-negative)
            -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // Triangle 1
            -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // Triangle 2

            // Right face (X-positive)
            1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // Triangle 1
            1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // Triangle 2

            // Bottom face (Y-negative)
            0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, // Triangle 1
            0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, // Triangle 2

            // Top face (Y-positive)
            0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // Triangle 1
            0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f  // Triangle 2
        };

        std::vector<float> textureUV = {
            // Back face
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,

            // Front face
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,

            // Left face
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,

            // Right face
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,

            // Bottom face
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,

            // Top face
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
        };

        std::vector<float> color = {
            // Back face (Red)
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            // Front face (Green)
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            // Left face (Blue)
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            // Right face (Yellow)
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            // Bottom face (Cyan)
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            // Top face (Magenta)
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
        };

        ECS::Global::get().registry->emplace<Components::RasterizerUniformBufferObject>(modelEntity, Components::RasterizerUniformBufferObject{});
        ECS::Global::get().registry->emplace<Components::ComputeUniformBufferObject>(modelEntity, Components::ComputeUniformBufferObject{});

        auto& rasterizerUniformBufferObject = ECS::Global::get().registry->get<Components::RasterizerUniformBufferObject>(modelEntity);
        auto& computeUniformBufferObject = ECS::Global::get().registry->get<Components::ComputeUniformBufferObject>(modelEntity);

        std::vector<uint32_t> indices =
            {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35};

        ECS::Global::get().registry->emplace<Components::MeshHost>(modelEntity, Components::MeshHost{
            .indices = indices,
            .positions = pos,
            .normals = normal,
            .texCoords = textureUV,
            .color = color,
            .boneIndices = {},
            .boneWeights = {}
        });

        ECS::Global::get().registry->emplace<Components::MeshDevice>(modelEntity, Components::MeshDevice{
            .positionsBuffer = HardwareBuffer(pos, BufferUsage::VertexBuffer),
            .normalsBuffer = HardwareBuffer(normal, BufferUsage::VertexBuffer),
            .texCoordsBuffer = HardwareBuffer(textureUV, BufferUsage::VertexBuffer),
            .colorBuffer = HardwareBuffer(color, BufferUsage::VertexBuffer),
            .computeUniformBuffer = HardwareBuffer(sizeof(computeUniformBufferObject), BufferUsage::UniformBuffer),
            .rasterizerUniformBuffer = HardwareBuffer(sizeof(rasterizerUniformBufferObject), BufferUsage::UniformBuffer),
            .boneIndicesBuffer = HardwareBuffer({}, BufferUsage::VertexBuffer),
            .boneWeightsBuffer = HardwareBuffer({}, BufferUsage::VertexBuffer)
        });


    }

} // namespace ECS