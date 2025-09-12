//
// Created by 47226 on 2025/9/8.
//

#pragma once
#include "Resource.h"

#include <complex.h>
#include <string>
#include <vector>

namespace Corona
{
    template <typename TRes>
        requires std::is_base_of_v<Resource, TRes> &&
                 std::default_initializable<TRes>
    class ResourceLoader
    {
      public:
        using Handle = std::shared_ptr<TRes>;

        virtual ~ResourceLoader() = default;
        // 修改这里：不再返回 Handle，而是通过引用修改传入的 handle
        virtual bool load(const std::string &path, const Handle &handle) = 0;
    };

} // namespace Corona
