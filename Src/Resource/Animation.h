#pragma once
#include "Bone.h"
#include "Core/IO/IResource.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

// 前置声明：避免在公开头中引入重量级头文件
struct aiScene;
struct aiAnimation;
struct aiNode;

#include <map>
#include <string>

namespace Corona
{

    class Animation final : public IResource
    {
      public:
        struct AssimpNodeData
        {
            ktm::fmat4x4 transformation;          // 节点变换矩阵
            std::string name;                     // 节点名称
            int childrenCount;                    // 子节点数量
            std::vector<AssimpNodeData> children; // 子节点数据
        };

        Animation() = default;
        ~Animation() = default;

        Animation(const aiScene *scene, const aiAnimation *animation, std::map<std::string, std::shared_ptr<BoneInfo>> boneInfoMap, int &boneCount);
        void readHeirarchyData(AssimpNodeData &dest, const aiNode *src);

        double m_Duration;                                            // 动画持续时间
        double m_TicksPerSecond;                                      // 每秒采样数
        std::vector<Bone> m_Bones;                                    // 动画涉及的所有骨骼
        AssimpNodeData m_RootNode;                                    // 根节点数据
        std::map<std::string, std::shared_ptr<BoneInfo>> boneInfoMap; // 骨骼信息映射表
    };

} // namespace Corona
