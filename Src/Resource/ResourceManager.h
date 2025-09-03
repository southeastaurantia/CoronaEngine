#pragma once

#include "ECS/Components.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <entt/entt.hpp>
#include <fstream>
#include <regex>
#include <string>
#include <filesystem>

// TODO: 资源缓存，资源uid
// TODO: 移除ECS操作, 解析后的资源数据不存储在ECS中, ECS中存储资源的uid 

namespace ECS
{
    class ResourceManager final
    {
      public:
        // TODO: Implement resource management functions
        static void LoadModel(entt::entity modelEntity, const std::string &filePath);
        static std::string readStringFile(std::string_view file_path);
        static void setBasePath(const std::string &path);
        static void setUserPath(const std::string &path);
        static void setShaderPath(const std::string &path);
        static std::string getBasePath();
        static std::string getUserPath();
        static std::string getShaderPath();

      private:
        static std::shared_ptr<entt::registry> registry;
        static std::string basePath;
        static std::string userPath;
        static std::string shaderPath;

        static void LoadAnimation(const aiScene *scene, aiAnimation *animation, entt::entity modelEntity);
        static void ReadHeirarchyData(Components::AssimpNodeData &dest, const aiNode *src);
        static void ProcessNode(std::string path, aiNode *node, const aiScene *scene, entt::entity modelEntity);
        static void ProcessMesh(std::string path, aiMesh *mesh, const aiScene *scene, entt::entity meshEntity, entt::entity modelEntity);
        static void ExtractBoneWeightForVertices(aiMesh *mesh, Components::MeshHost &meshHost, const aiScene *scene, entt::entity modelEntity);
        static void LoadMaterial(std::string path, aiMaterial *material, entt::entity modelEntity);
        static entt::entity createTextureEntity(const std::string &texturePath, aiTextureType textureType);
        static entt::entity createColorTextureEntity(const std::string &directory, aiTextureType textureType, const aiColor3D &color);
        static void ReadBoneChannels(aiAnimation *animation, std::vector<Components::Bone> &outBones, std::map<std::string, Components::BoneInfo> &boneInfoMap, int &boneCount);
        static void LoadKeyPositions(aiNodeAnim *channel, std::vector<Components::KeyPosition> &outPositions);
        static void LoadKeyRotations(aiNodeAnim *channel, std::vector<Components::KeyRotation> &outRotations);
        static void LoadKeyScales(aiNodeAnim *channel, std::vector<Components::KeyScale> &outScales);
    };


  inline std::string ResourceManager::shaderPath = [] {
    std::string resultPath = "";
    const std::string runtimePath = std::filesystem::current_path().string();
    // std::replace(runtimePath.begin(), runtimePath.end(), '\\', '/');
    const std::regex pattern(R"((.*)CabbageFramework\b)");
    if (std::smatch matches; std::regex_search(runtimePath, matches, pattern))
    {
        if (matches.size() > 1)
        {
            resultPath = matches[1].str() + "CabbageFramework";
        }
        else
        {
            throw std::runtime_error("Failed to resolve source path.");
        }
    }
    std::ranges::replace(resultPath, '\\', '/');
    return resultPath + "/Examples/assest";
  }();
} // namespace ECS