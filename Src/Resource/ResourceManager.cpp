#include "ResourceManager.h"

#include <ECS/Core.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>

// namespace ECS
// {
//     void ResourceManager::LoadModel(const entt::entity modelEntity, const std::string &filePath)
//     {
//         Assimp::Importer importer{};
//
//         const aiScene *scene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);
//
//         if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
//         {
//             LOG_ERROR(std::format("Assimp Error: {}", importer.GetErrorString()));
//             return;
//         }
//
//         {
//             registry->emplace<Components::Meshes>(modelEntity, Components::Meshes{
//                                                                    .data = {},
//                                                                    .path = filePath});
//
//             registry->emplace_or_replace<Components::Animations>(modelEntity, Components::Animations{
//                                                                                   .skeletalAnimations = {},
//                                                                                   .boneInfoMap = {},
//                                                                                   .boneCount = 0});
//
//             ProcessNode(filePath, scene->mRootNode, scene, modelEntity);
//
//             auto &animationsEntity = registry->get<Components::Animations>(modelEntity);
//             animationsEntity.skeletalAnimations.reserve(scene->mNumAnimations);
//             for (unsigned int i = 0; i < scene->mNumAnimations; i++)
//             {
//                 LoadAnimation(scene, scene->mAnimations[i], modelEntity);
//             }
//
//             auto &meshes = registry->get<Components::Meshes>(modelEntity);
//             if (meshes.data.size() > 0)
//             {
//                 //for(const auto &meshEntity : meshes.data)
//                 //{
//                 //    if(registry->try_get<Components::AABB>(meshEntity))
//                 //    {
//                 //        const auto &meshAABB = registry->get<Components::AABB>(meshEntity);
//                 //        meshAABB.aabbMaxXYZ = ktm::fvec3(ktm::max())
//                 //    }
//                 //}
//             }
//
//             registry->emplace<Components::ResLoadedTag>(modelEntity);
//         }
//     }
//
//     std::string ResourceManager::readStringFile(const std::string_view file_path)
//     {
//         std::ifstream file(file_path.data());
//         if (!file.is_open())
//         {
//             throw std::runtime_error("Could not open the file.");
//         }
//
//         std::stringstream buffer;
//         buffer << file.rdbuf();
//
//         file.close();
//         return buffer.str();
//     }
//
//     void ResourceManager::setBasePath(const std::string &path)
//     {
//         basePath = path;
//     }
//
//     void ResourceManager::setUserPath(const std::string &path)
//     {
//         userPath = path;
//     }
//
//     void ResourceManager::setShaderPath(const std::string &path)
//     {
//         shaderPath = path;
//     }
//
//     std::string ResourceManager::getBasePath()
//     {
//         return basePath;
//     }
//
//     std::string ResourceManager::getUserPath()
//     {
//         return userPath;
//     }
//
//     std::string ResourceManager::getShaderPath()
//     {
//         return shaderPath;
//     }
//
//     void ResourceManager::LoadAnimation(const aiScene *scene, aiAnimation *animation, entt::entity modelEntity)
//     {
//         auto &animations = registry->get<Components::Animations>(modelEntity);
//         Components::SkeletalAnimation skeletalAnimation;
//
//         skeletalAnimation.duration = animation->mDuration;
//         skeletalAnimation.ticksPerSecond = animation->mTicksPerSecond;
//
//         ReadHeirarchyData(skeletalAnimation.rootNode, scene->mRootNode);
//
//         ReadBoneChannels(animation, skeletalAnimation.bones, animations.boneInfoMap, animations.boneCount);
//     }
//
//     void ResourceManager::ReadHeirarchyData(Components::AssimpNodeData &dest, const aiNode *src)
//     {
//         auto ConvertMatrixToGLMFormat = [](const aiMatrix4x4 &from) -> ktm::fmat4x4 {
//             ktm::fmat4x4 to;
//             to[0][0] = from.a1;
//             to[1][0] = from.a2;
//             to[2][0] = from.a3;
//             to[3][0] = from.a4;
//             to[0][1] = from.b1;
//             to[1][1] = from.b2;
//             to[2][1] = from.b3;
//             to[3][1] = from.b4;
//             to[0][2] = from.c1;
//             to[1][2] = from.c2;
//             to[2][2] = from.c3;
//             to[3][2] = from.c4;
//             to[0][3] = from.d1;
//             to[1][3] = from.d2;
//             to[2][3] = from.d3;
//             to[3][3] = from.d4;
//             return to;
//         };
//
//         dest.name = src->mName.data;
//         dest.transformation = ConvertMatrixToGLMFormat(src->mTransformation);
//         dest.childrenCount = src->mNumChildren;
//
//         for (uint32_t i = 0; i < src->mNumChildren; i++)
//         {
//             Components::AssimpNodeData newData;
//             ReadHeirarchyData(newData, src->mChildren[i]);
//             dest.children.push_back(newData);
//         }
//     }
//
//     void ResourceManager::ReadBoneChannels(aiAnimation *animation, std::vector<Components::Bone> &outBones,
//                                            std::map<std::string, Components::BoneInfo> &boneInfoMap, int &boneCount)
//     {
//         for (uint32_t i = 0; i < animation->mNumChannels; i++)
//         {
//             auto channel = animation->mChannels[i];
//             std::string boneName = channel->mNodeName.data;
//             if (boneInfoMap.find(boneName) == boneInfoMap.end())
//             {
//                 boneInfoMap[boneName].id = boneCount;
//                 boneCount++;
//             }
//
//             Components::Bone bone;
//             bone.name = boneName;
//             bone.id = boneInfoMap[boneName].id;
//             LoadKeyPositions(channel, bone.keyPositions);
//             bone.NumPositions = bone.keyPositions.size();
//
//             LoadKeyRotations(channel, bone.keyRotations);
//             bone.NumRotations = bone.keyRotations.size();
//
//             LoadKeyScales(channel, bone.keyScales);
//             bone.NumScales = bone.keyScales.size();
//
//             outBones.push_back(bone);
//         }
//     }
//
//     void ResourceManager::LoadKeyPositions(aiNodeAnim *channel, std::vector<Components::KeyPosition> &outPositions)
//     {
//         for (uint32_t i = 0; i < channel->mNumPositionKeys; i++)
//         {
//             Components::KeyPosition keyPosition;
//             keyPosition.time = channel->mPositionKeys[i].mTime;
//             keyPosition.position = {
//                 channel->mPositionKeys[i].mValue.x,
//                 channel->mPositionKeys[i].mValue.y,
//                 channel->mPositionKeys[i].mValue.z};
//             outPositions.push_back(keyPosition);
//         }
//     }
//
//     void ResourceManager::LoadKeyRotations(aiNodeAnim *channel, std::vector<Components::KeyRotation> &outRotations)
//     {
//         for (uint32_t i = 0; i < channel->mNumRotationKeys; i++)
//         {
//             Components::KeyRotation keyRotation;
//             keyRotation.time = channel->mRotationKeys[i].mTime;
//             keyRotation.rotation = {
//                 channel->mRotationKeys[i].mValue.x,
//                 channel->mRotationKeys[i].mValue.y,
//                 channel->mRotationKeys[i].mValue.z,
//                 channel->mRotationKeys[i].mValue.w};
//             outRotations.push_back(keyRotation);
//         }
//     }
//
//     void ResourceManager::LoadKeyScales(aiNodeAnim *channel, std::vector<Components::KeyScale> &outScales)
//     {
//         for (uint32_t i = 0; i < channel->mNumScalingKeys; i++)
//         {
//             Components::KeyScale keyScale;
//             keyScale.time = channel->mScalingKeys[i].mTime;
//             keyScale.scale = {
//                 channel->mScalingKeys[i].mValue.x,
//                 channel->mScalingKeys[i].mValue.y,
//                 channel->mScalingKeys[i].mValue.z};
//             outScales.push_back(keyScale);
//         }
//     }
//
//     void ResourceManager::ProcessNode(std::string path, aiNode *node, const aiScene *scene, entt::entity modelEntity)
//     {
//         for (unsigned int i = 0; i < node->mNumMeshes; i++)
//         {
//             aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
//
//             auto meshEntity = registry->create();
//             ProcessMesh(path, mesh, scene, meshEntity, modelEntity);
//
//             auto &meshes = registry->get<Components::Meshes>(modelEntity);
//             meshes.data.push_back(meshEntity);
//         }
//
//         for (unsigned int i = 0; i < node->mNumChildren; i++)
//         {
//             ProcessNode(path, node->mChildren[i], scene, modelEntity);
//         }
//     }
//
//     void ResourceManager::ProcessMesh(std::string path, aiMesh *mesh, const aiScene *scene, entt::entity meshEntity, entt::entity modelEntity)
//     {
//         auto &meshHost = registry->emplace<Components::MeshHost>(meshEntity);
//
//         meshHost.positions.reserve(mesh->mNumVertices * 3);
//         meshHost.normals.reserve(mesh->mNumVertices * 3);
//         meshHost.texCoords.reserve(mesh->mNumVertices * 2);
//         meshHost.boneIndices.resize(mesh->mNumVertices * 4, 0);
//         meshHost.boneWeights.resize(mesh->mNumVertices * 4, 0.0f);
//
//         for (unsigned int i = 0; i < mesh->mNumVertices; i++)
//         {
//             meshHost.positions.push_back(mesh->mVertices[i].x);
//             meshHost.positions.push_back(mesh->mVertices[i].y);
//             meshHost.positions.push_back(mesh->mVertices[i].z);
//
//             if (mesh->HasNormals())
//             {
//                 meshHost.normals.push_back(mesh->mNormals[i].x);
//                 meshHost.normals.push_back(mesh->mNormals[i].y);
//                 meshHost.normals.push_back(mesh->mNormals[i].z);
//             }
//
//             if (mesh->HasTextureCoords(0))
//             {
//                 meshHost.texCoords.push_back(mesh->mTextureCoords[0][i].x);
//                 meshHost.texCoords.push_back(mesh->mTextureCoords[0][i].y);
//             }
//             else
//             {
//                 meshHost.texCoords.push_back(0.0f);
//                 meshHost.texCoords.push_back(0.0f);
//             }
//         }
//
//         meshHost.boneIndices.resize(mesh->mNumVertices * 4, 0);
//         meshHost.boneWeights.resize(mesh->mNumVertices * 4, 0.0f);
//
//         ExtractBoneWeightForVertices(mesh, meshHost, scene, modelEntity);
//
//         meshHost.indices.reserve(mesh->mNumFaces * 3);
//         for (unsigned int i = 0; i < mesh->mNumFaces; i++)
//         {
//             aiFace face = mesh->mFaces[i];
//             for (unsigned int j = 0; j < face.mNumIndices; j++)
//             {
//                 meshHost.indices.push_back(face.mIndices[j]);
//             }
//         }
//
//         registry->emplace<Components::MeshDevice>(meshEntity, Components::MeshDevice{
//                                                                   .indicesBuffer = HardwareBuffer(meshHost.indices, BufferUsage::IndexBuffer),
//                                                                   .positionsBuffer = HardwareBuffer(meshHost.positions, BufferUsage::VertexBuffer),
//                                                                   .normalsBuffer = HardwareBuffer(meshHost.normals, BufferUsage::VertexBuffer),
//                                                                   .texCoordsBuffer = HardwareBuffer(meshHost.texCoords, BufferUsage::VertexBuffer),
//                                                                   .boneIndicesBuffer = HardwareBuffer(meshHost.boneIndices, BufferUsage::VertexBuffer),
//                                                                   .boneWeightsBuffer = HardwareBuffer(meshHost.boneWeights, BufferUsage::VertexBuffer),
//                                                               });
//
//         if (mesh->mMaterialIndex >= 0)
//         {
//             LoadMaterial(path, scene->mMaterials[mesh->mMaterialIndex], modelEntity);
//         }
//     }
//
//     void ResourceManager::ExtractBoneWeightForVertices(aiMesh *mesh, Components::MeshHost &meshHost, const aiScene *scene, entt::entity modelEntity)
//     {
//         auto &animations = registry->get<Components::Animations>(modelEntity);
//         auto &boneInfoMap = animations.boneInfoMap;
//         int &boneCount = animations.boneCount;
//
//         auto ConvertMatrixToKTFormat = [](const aiMatrix4x4 &from) -> ktm::fmat4x4 {
//             ktm::fmat4x4 to;
//             to[0][0] = from.a1;
//             to[1][0] = from.a2;
//             to[2][0] = from.a3;
//             to[3][0] = from.a4;
//             to[0][1] = from.b1;
//             to[1][1] = from.b2;
//             to[2][1] = from.b3;
//             to[3][1] = from.b4;
//             to[0][2] = from.c1;
//             to[1][2] = from.c2;
//             to[2][2] = from.c3;
//             to[3][2] = from.c4;
//             to[0][3] = from.d1;
//             to[1][3] = from.d2;
//             to[2][3] = from.d3;
//             to[3][3] = from.d4;
//             return to;
//         };
//
//         for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
//         {
//             int boneID = -1;
//             std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
//
//             if (boneInfoMap.find(boneName) == boneInfoMap.end())
//             {
//                 boneInfoMap[boneName] = Components::BoneInfo{
//                     .id = boneCount,
//                     .offsetMatrix = ConvertMatrixToKTFormat(mesh->mBones[boneIndex]->mOffsetMatrix)};
//                 boneID = boneCount;
//                 boneCount++;
//             }
//             else
//             {
//                 boneID = boneInfoMap[boneName].id;
//             }
//
//             auto weights = mesh->mBones[boneIndex]->mWeights;
//             int numWeights = mesh->mBones[boneIndex]->mNumWeights;
//
//             for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
//             {
//                 int vertexId = weights[weightIndex].mVertexId;
//                 float weight = weights[weightIndex].mWeight;
//
//                 for (int i = 0; i < 4; ++i)
//                 {
//                     size_t index = vertexId * 4 + i;
//                     if (index < meshHost.boneWeights.size() && meshHost.boneWeights[index] <= 1e-3)
//                     {
//                         meshHost.boneWeights[index] = weight;
//                         meshHost.boneIndices[index] = boneID;
//                         break;
//                     }
//                 }
//             }
//         }
//     }
//
//     void ResourceManager::LoadMaterial(std::string path, aiMaterial *material, entt::entity modelEntity)
//     {
//         std::string meshRootPath = "";
//         std::regex rexPattern("(.*)((\\\\)|(/))");
//         std::smatch matchPath;
//         if (regex_search(path, matchPath, rexPattern))
//         {
//             meshRootPath = matchPath[1].str();
//         }
//         else
//         {
//             throw "mesh path invalid in python";
//         }
//         std::string directory = meshRootPath;
//
//         entt::entity materialEntity = registry->create();
//
//         registry->emplace<Components::Material>(modelEntity, Components::Material{.material = materialEntity});
//
//         std::vector<aiTextureType> allTextureTypes = {
//             aiTextureType_BASE_COLOR,
//             aiTextureType_DIFFUSE,
//             aiTextureType_SPECULAR,
//             aiTextureType_EMISSIVE};
//
//         for (aiTextureType textureType : allTextureTypes)
//         {
//             for (unsigned int i = 0; i < material->GetTextureCount(textureType); i++)
//             {
//                 aiString str;
//                 if (material->GetTexture(textureType, i, &str) != aiReturn_SUCCESS)
//                     continue;
//
//                 std::string texturePath = directory + str.C_Str();
//                 entt::entity textureEntity = createTextureEntity(texturePath, textureType);
//
//                 switch (textureType)
//                 {
//                 case aiTextureType_BASE_COLOR:
//                     registry->emplace<Components::BaseColorTexture>(materialEntity, Components::BaseColorTexture{
//                                                                                         .texture = textureEntity});
//                     break;
//                 case aiTextureType_DIFFUSE:
//                     break;
//                 case aiTextureType_SPECULAR:
//                     break;
//                 case aiTextureType_EMISSIVE:
//                     break;
//                 default:
//                     break;
//                 }
//             }
//         }
//
//         Components::MaterialParams materialParams;
//
//         aiColor3D baseColor(0.f, 0.f, 0.f);
//         if (material->Get(AI_MATKEY_BASE_COLOR, baseColor) == aiReturn_SUCCESS)
//         {
//             materialParams.baseColor = {baseColor[0], baseColor[1], baseColor[2]};
//
//             if (registry->try_get<Components::BaseColorTexture>(materialEntity))
//             {
//                 entt::entity colorTextureEntity = createColorTextureEntity(directory, aiTextureType_BASE_COLOR, baseColor);
//                 registry->emplace<Components::BaseColorTexture>(materialEntity, Components::BaseColorTexture{.texture = colorTextureEntity});
//             }
//         }
//
//         float metallic = 0.0f;
//         if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == aiReturn_SUCCESS)
//         {
//             materialParams.metallic = metallic;
//         }
//
//         float roughness = 0.0f;
//         if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == aiReturn_SUCCESS)
//         {
//             materialParams.roughness = roughness;
//         }
//
//         float specular = 0.0f;
//         if (material->Get(AI_MATKEY_SPECULAR_FACTOR, specular) == aiReturn_SUCCESS)
//         {
//             materialParams.specular = specular;
//         }
//
//         float transmission = 0.0f;
//         if (material->Get(AI_MATKEY_OPACITY, transmission) == aiReturn_SUCCESS)
//         {
//             materialParams.transmission = 1.0f - transmission;
//         }
//
//         registry->emplace<Components::MaterialParams>(materialEntity, materialParams);
//     }
//
//     entt::entity ResourceManager::createTextureEntity(const std::string &texturePath, aiTextureType textureType)
//     {
//         entt::entity textureEntity = registry->create();
//
//         static std::unordered_map<std::string, entt::entity> loadedTextures;
//         auto it = loadedTextures.find(texturePath);
//
//         if (it != loadedTextures.end())
//         {
//             return it->second;
//         }
//
//         int width, height, channels;
//         unsigned char *data = stbi_load(texturePath.c_str(), &width, &height, &channels, 0);
//
//         if (!data)
//         {
//             std::cerr << "Failed to load texture: " << texturePath << std::endl;
//             return entt::null;
//         }
//
//         registry->emplace<Components::ImageHost>(textureEntity, Components::ImageHost{
//                                                                     .path = texturePath,
//                                                                     .data = data,
//                                                                     .width = width,
//                                                                     .height = height,
//                                                                     .channels = channels});
//
//         loadedTextures[texturePath] = textureEntity;
//
//         return textureEntity;
//     }
//
//     entt::entity ResourceManager::createColorTextureEntity(const std::string &directory, aiTextureType textureType, const aiColor3D &color)
//     {
//         entt::entity textureEntity = registry->create();
//
//         static int colorTextureCounter = 0;
//         std::string texturePath = directory + "color_" + std::to_string(textureType) + "_" + std::to_string(++colorTextureCounter);
//
//         unsigned char *data = (unsigned char *)malloc(sizeof(unsigned char) * 4);
//         if (data)
//         {
//             data[0] = static_cast<unsigned char>(color[0] * 255.0f);
//             data[1] = static_cast<unsigned char>(color[1] * 255.0f);
//             data[2] = static_cast<unsigned char>(color[2] * 255.0f);
//             data[3] = 255;
//
//             registry->emplace<Components::ImageHost>(textureEntity, Components::ImageHost{
//                                                                         .path = texturePath,
//                                                                         .data = data,
//                                                                         .width = 1,
//                                                                         .height = 1,
//                                                                         .channels = 4});
//         }
//
//         return textureEntity;
//     }
//
// } // namespace ECS
CoronaEngine::ResourceManager::ResourceManager() = default;
CoronaEngine::ResourceManager::~ResourceManager()
{
    tbb_task_group.wait();
}
CoronaEngine::ResourceManager &CoronaEngine::ResourceManager::get_singleton()
{
    static ResourceManager inst;
    return inst;
}