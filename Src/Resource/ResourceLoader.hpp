//
// Created by 47226 on 2025/9/7.
//

#ifndef CORONAENGINE_RESOURCELOADER_HPP
#define CORONAENGINE_RESOURCELOADER_HPP
#include "Resource.hpp"

namespace Corona
{
    // 类型擦除
    class IResourceLoader
    {
      public:
        virtual ~IResourceLoader() = default;

        virtual bool load(const std::string &path, std::shared_ptr<Resource> resource) = 0;
    };

    template <typename ResourceType>
    class ResourceLoader : public IResourceLoader
    {
        static_assert(std::is_base_of_v<Resource, ResourceType>, "ResourceType must be derived from Resource");

      public:
        using ResourceHandle = std::shared_ptr<ResourceType>;

        ~ResourceLoader() override = default;
        // 重载此方法实现具体资源加载逻辑
        virtual bool on_load(const std::string &path, ResourceHandle resource) = 0;

        bool load(const std::string &path, std::shared_ptr<Resource> resource) override
        {
            return this->on_load(path, std::static_pointer_cast<ResourceType>(resource));
        }
    };

} // namespace CoronaEngine

#endif // CORONAENGINE_RESOURCELOADER_HPP
