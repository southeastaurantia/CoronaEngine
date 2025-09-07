#pragma once
#include "Resource.hpp"
#include "ResourceLoader.hpp"
#include "Utils/CabbageLogger.hpp"
#include "oneapi/tbb/concurrent_unordered_map.h"
#include "oneapi/tbb/task_group.h"

#include <any>
#include <future>
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
        // 使用 std::any 来存储不同资源类型的 future，以实现类型擦除
        using ResourceCache = tbb::concurrent_unordered_map<std::string, std::any>;
        using LoaderCache = std::unordered_map<std::type_index, std::shared_ptr<IResourceLoader>>;

      public:
        explicit ResourceManager();
        ~ResourceManager();

        static ResourceManager &get_singleton();

        template <typename ResourceType>
            requires std::is_base_of_v<Resource, ResourceType> && std::default_initializable<ResourceType>
        std::shared_future<typename ResourceLoader<ResourceType>::ResourceHandle> load(const std::string &path)
        {
            using ResourceHandle = typename ResourceLoader<ResourceType>::ResourceHandle;

            // 1. 检查资源是否已在缓存中
            if (auto it = res_caches.find(path); it != res_caches.end())
            {
                LOG_DEBUG(std::format("Get cache resource future: {}", path));
                try
                {
                    // 尝试将 std::any 转换为具体的 future 类型
                    return std::any_cast<std::shared_future<ResourceHandle>>(it->second);
                }
                catch (const std::bad_any_cast &e)
                {
                    // 如果转换失败，说明同一路径被请求为不同的资源类型，这是一个严重错误
                    auto error_msg = std::format("Resource type mismatch for path: {}. Requested type does not match cached type.", path);
                    LOG_ERROR(error_msg);
                    auto promise = std::make_shared<std::promise<ResourceHandle>>();
                    promise->set_exception(std::make_exception_ptr(std::runtime_error(error_msg)));
                    return promise->get_future().share();
                }
            }

            // 2. 如果资源未缓存，则创建一个新的 promise 和 future
            auto promise = std::make_shared<std::promise<ResourceHandle>>();
            std::shared_future<ResourceHandle> future = promise->get_future().share();

            // 3. 尝试将新创建的 future 插入缓存。这是一个原子操作，可以防止竞态条件
            auto [iter, inserted] = res_caches.emplace(path, future);

            if (inserted)
            {
                // 如果插入成功，说明当前线程是第一个请求该资源的线程
                LOG_DEBUG(std::format("Creating new loading task for: {}", path));
                const auto res_type_id = std::type_index(typeid(ResourceType));
                auto loader_it = res_loaders.find(res_type_id);

                if (loader_it == res_loaders.end())
                {
                    auto error_msg = std::format("Resource type '{}' load failed: Not register resource loader", res_type_id.name());
                    LOG_ERROR(error_msg);
                    promise->set_exception(std::make_exception_ptr(std::runtime_error(error_msg)));
                    return future;
                }

                auto loader = loader_it->second;

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
                try
                {
                    // 返回已存在于缓存中的 future
                    return std::any_cast<std::shared_future<ResourceHandle>>(iter->second);
                }
                catch (const std::bad_any_cast &e)
                {
                    auto error_msg = std::format("Resource type mismatch for path: {}. Requested type does not match cached type.", path);
                    LOG_ERROR(error_msg);
                    auto p_err = std::make_shared<std::promise<ResourceHandle>>();
                    p_err->set_exception(std::make_exception_ptr(std::runtime_error(error_msg)));
                    return p_err->get_future().share();
                }
            }
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