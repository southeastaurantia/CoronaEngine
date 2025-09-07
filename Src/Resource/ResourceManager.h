#pragma once
#include "Resource.hpp"
#include "ResourceLoader.hpp"
#include "Utils/CabbageLogger.hpp"
#include "oneapi/tbb/concurrent_unordered_map.h"
#include "oneapi/tbb/task_group.h"

#include <future>

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
    template <typename ResourceType>
        requires std::is_base_of_v<Resource, ResourceType> && std::default_initializable<ResourceType>
    class ResourceManager final
    {
        using ResourceHandle = typename ResourceLoader<ResourceType>::ResourceHandle;
        using ResourceFuture = std::shared_future<ResourceHandle>;
        using ResourceCache = tbb::concurrent_unordered_map<std::string, ResourceFuture>;

      public:
        explicit ResourceManager() = default;
        ~ResourceManager()
        {
            tbb_task_group.wait();
        }

        // 为每种资源类型提供一个单例
        static ResourceManager &get_singleton()
        {
            static ResourceManager inst;
            return inst;
        }

        ResourceFuture load(const std::string &path)
        {
            // 1. 检查资源是否已在缓存中
            if (auto it = res_caches.find(path); it != res_caches.end())
            {
                LOG_DEBUG(std::format("Get cache resource future: {}", path));
                return it->second;
            }

            // 2. 如果资源未缓存，则创建一个新的 promise 和 future
            auto promise = std::make_shared<std::promise<ResourceHandle>>();
            ResourceFuture future = promise->get_future().share();

            // 3. 尝试将新创建的 future 插入缓存。这是一个原子操作，可以防止竞态条件
            auto [iter, inserted] = res_caches.emplace(path, future);

            if (inserted)
            {
                // 如果插入成功，说明当前线程是第一个请求该资源的线程
                LOG_DEBUG(std::format("Creating new loading task for: {}", path));

                if (!res_loader)
                {
                    auto error_msg = std::format("Resource type '{}' load failed: Not register resource loader", typeid(ResourceType).name());
                    LOG_ERROR(error_msg);
                    promise->set_exception(std::make_exception_ptr(std::runtime_error(error_msg)));
                    return future;
                }

                auto loader = res_loader;

                // 4. 在TBB任务组中异步执行资源加载
                tbb_task_group.run([path, loader, promise] {
                    try
                    {
                        auto res = std::make_shared<ResourceType>();
                        res->res_id = path;
                        res->res_status.store(Resource::Status::LOADING);

                        if (loader->load(path, res))
                        {
                            res->res_status.store(Resource::Status::OK);
                            promise->set_value(res); // 加载成功，设置 promise 的值
                        }
                        else
                        {
                            res->res_status.store(Resource::Status::FAILED);
                            throw std::runtime_error(std::format("Resource loader failed for path: {}", path));
                        }
                    }
                    catch (...)
                    {
                        // 捕获任何异常，并通过 promise 传递
                        promise->set_exception(std::current_exception());
                    }
                });
                return future;
            }
            else
            {
                // 如果插入失败，说明有其他线程已经创建了该资源的加载任务
                LOG_DEBUG(std::format("Another thread is already loading resource: {}", path));
                // 返回已存在于缓存中的 future
                return iter->second;
            }
        }

        template <typename LoaderType>
            requires std::is_base_of_v<ResourceLoader<ResourceType>, LoaderType> &&
                     std::default_initializable<LoaderType>
        void register_loader()
        {
            if (res_loader)
                return;
            res_loader = std::make_shared<LoaderType>();
            LOG_DEBUG(std::format("Register resource loader '{}' for resource type '{}'",
                                  typeid(LoaderType).name(), typeid(ResourceType).name()));
        }

      private:
        std::shared_ptr<IResourceLoader> res_loader{};
        ResourceCache res_caches{};
        tbb::task_group tbb_task_group{};
    };
} // namespace CoronaEngine