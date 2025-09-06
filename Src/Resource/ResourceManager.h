#pragma once
#include "Resource.hpp"
#include "ResourceLoader.hpp"
#include "Utils/CabbageLogger.hpp"
#include "oneapi/tbb/concurrent_unordered_map.h"
#include "oneapi/tbb/task_group.h"

#include <typeindex>

// TODO: 资源缓存，资源uid
// TODO: 移除ECS操作, 解析后的资源数据不存储在ECS中, ECS中存储资源的uid

// namespace ECS
// {
//     class ResourceManager final
//     {
// public:
//   // TODO: Implement resource management functions
//   static void LoadModel(entt::entity modelEntity, const std::string &filePath);
//   static std::string readStringFile(std::string_view file_path);
//   static void setBasePath(const std::string &path);
//   static void setUserPath(const std::string &path);
//   static void setShaderPath(const std::string &path);
//   static std::string getBasePath();
//   static std::string getUserPath();
//   static std::string getShaderPath();
//
// private:
//   static std::shared_ptr<entt::registry> registry;
//   static std::string basePath;
//   static std::string userPath;
//   static std::string shaderPath;
//
//   static void LoadAnimation(const aiScene *scene, aiAnimation *animation, entt::entity modelEntity);
//   static void ReadHeirarchyData(Components::AssimpNodeData &dest, const aiNode *src);
//   static void ProcessNode(std::string path, aiNode *node, const aiScene *scene, entt::entity modelEntity);
//   static void ProcessMesh(std::string path, aiMesh *mesh, const aiScene *scene, entt::entity meshEntity, entt::entity modelEntity);
//   static void ExtractBoneWeightForVertices(aiMesh *mesh, Components::MeshHost &meshHost, const aiScene *scene, entt::entity modelEntity);
//   static void LoadMaterial(std::string path, aiMaterial *material, entt::entity modelEntity);
//   static entt::entity createTextureEntity(const std::string &texturePath, aiTextureType textureType);
//   static entt::entity createColorTextureEntity(const std::string &directory, aiTextureType textureType, const aiColor3D &color);
//   static void ReadBoneChannels(aiAnimation *animation, std::vector<Components::Bone> &outBones, std::map<std::string, Components::BoneInfo> &boneInfoMap, int &boneCount);
//   static void LoadKeyPositions(aiNodeAnim *channel, std::vector<Components::KeyPosition> &outPositions);
//   static void LoadKeyRotations(aiNodeAnim *channel, std::vector<Components::KeyRotation> &outRotations);
//   static void LoadKeyScales(aiNodeAnim *channel, std::vector<Components::KeyScale> &outScales);
//     };
//
//
//   inline std::string ResourceManager::shaderPath = [] {
//     std::string resultPath = "";
//     const std::string runtimePath = std::filesystem::current_path().string();
//     // std::replace(runtimePath.begin(), runtimePath.end(), '\\', '/');
//     const std::regex pattern(R"((.*)CabbageFramework\b)");
//     if (std::smatch matches; std::regex_search(runtimePath, matches, pattern))
//     {
//         if (matches.size() > 1)
//         {
//             resultPath = matches[1].str() + "CabbageFramework";
//         }
//         else
//         {
//             throw std::runtime_error("Failed to resolve source path.");
//         }
//     }
//     std::ranges::replace(resultPath, '\\', '/');
//     return resultPath + "/Examples/assest";
//   }();
// } // namespace ECS

namespace CoronaEngine
{
    class ResourceManager final
    {
        using ResourceCache = tbb::concurrent_unordered_map<std::string, std::shared_ptr<Resource>>;
        using LoaderCache = std::unordered_map<std::type_index, std::shared_ptr<IResourceLoader>>;

      public:
        explicit ResourceManager();
        ~ResourceManager();

        static ResourceManager &get_singleton();

        template <typename ResourceType>
            requires std::is_base_of_v<Resource, ResourceType> &&
                     std::default_initializable<ResourceType>
        ResourceLoader<ResourceType>::ResourceHandle load(const std::string &path)
        {
            // 资源已缓存
            if (res_caches.contains(path))
            {
                LOG_DEBUG(std::format("Get cache resource: {}", path));
                return std::static_pointer_cast<ResourceType>(res_caches[path]);
            }

            const auto res_type_id = std::type_index(typeid(ResourceType));
            auto loader = res_loaders.find(res_type_id);
            // 未注册加载器
            if (loader == res_loaders.end())
            {
                LOG_ERROR(std::format("Resource type '{}' load failed: Not register resource loader", res_type_id.name()));
                return nullptr;
            }

            auto res = std::make_shared<ResourceType>();
            res->res_id = path;
            res->res_status.store(Resource::Status::LOADING);

            res_caches.insert(std::make_pair(res->res_id, res));
            tbb_task_group.run([this, loader, res] {
                if (loader->second->load(res->res_id, res))
                {
                    res->res_status.store(Resource::Status::OK);
                }
                else
                {
                    res->res_status.store(Resource::Status::FAILED);
                }
            });

            return res;
        }

        template <typename ResourceType, typename LoaderType>
            requires std::is_base_of_v<Resource, ResourceType> &&
                     std::is_base_of_v<ResourceLoader<ResourceType>, LoaderType> &&
                     std::default_initializable<ResourceType> &&
                     std::default_initializable<LoaderType>
        void register_loader()
        {
            const auto id = std::type_index(typeid(ResourceType));
            if (res_loaders.contains(id))
                return;
            res_loaders.insert(std::make_pair(id, std::static_pointer_cast<IResourceLoader>(std::make_shared<LoaderType>())));
            LOG_DEBUG(std::format("Register resource loader '{}'", std::type_index(typeid(LoaderType)).name()));
        }

      private:
        LoaderCache res_loaders{};
        ResourceCache res_caches{};
        tbb::task_group tbb_task_group{};
    };
} // namespace CoronaEngine