#pragma once

#include "ECS/Components.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <entt/entt.hpp>
#include <fstream>
#include <string>
#include <regex>

namespace ECS
{
    class ResourceManager final
    {
      public:
        explicit ResourceManager(std::shared_ptr<entt::registry> registry);
        ~ResourceManager();

        // TODO: Implement resource management functions
        void LoadModel(entt::entity modelEntity, const std::string &filePath);
        std::string readStringFile(const std::string_view file_path);
        void setBasePath(const std::string& basePath);
        void setUserPath(const std::string& userPath);
        std::string getBasePath() const;
        std::string getUserPath() const;
        
      private:
        std::shared_ptr<entt::registry> registry;
        std::string basePath;
        std::string userPath;
        
        void LoadAnimation(const aiScene *scene, aiAnimation *animation, entt::entity modelEntity);
        void ReadHeirarchyData(Components::AssimpNodeData &dest, const aiNode *src);
        void ProcessNode(std::string path, aiNode *node, const aiScene *scene, entt::entity modelEntity);
        void ProcessMesh(std::string path, aiMesh *mesh, const aiScene *scene, entt::entity meshEntity, entt::entity modelEntity);
        void ExtractBoneWeightForVertices(aiMesh *mesh, Components::MeshHost &meshHost, const aiScene *scene, entt::entity modelEntity);
        void LoadMaterial(std::string path, aiMaterial *material, entt::entity modelEntity);
        entt::entity createTextureEntity(const std::string& texturePath, aiTextureType textureType);
        entt::entity createColorTextureEntity(const std::string& directory, aiTextureType textureType, const aiColor3D& color);
        void ReadBoneChannels(aiAnimation *animation, std::vector<Components::Bone>& outBones, std::map<std::string, Components::BoneInfo>& boneInfoMap, int& boneCount);
        void LoadKeyPositions(aiNodeAnim* channel, std::vector<Components::KeyPosition>& outPositions);
        void LoadKeyRotations(aiNodeAnim* channel, std::vector<Components::KeyRotation>& outRotations);
        void LoadKeyScales(aiNodeAnim* channel, std::vector<Components::KeyScale>& outScales);
    };
} // namespace ECS