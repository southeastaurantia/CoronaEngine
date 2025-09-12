#pragma once

namespace Corona
{
    // 基类：所有资源对象需继承，便于类型擦除与统一缓存
    struct IResource
    {
        virtual ~IResource() = default;
    };
} // namespace Corona
