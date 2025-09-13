#pragma once
#include "Core/IO/IResource.h"
// forward declare to avoid heavy include in header
struct aiNodeAnim;
// 需要使用 ktm 的数学类型定义（fvec3/fquat/fmat4x4）
#include "ktm/type_mat.h"
#include "ktm/type_quat.h"
#include "ktm/type_vec.h"
#include <vector>

#include <string>

namespace Corona
{
    class Bone final : public IResource
    {
      public:
        Bone() = default;
        ~Bone() = default;

        Bone(const std::string &name, const int ID, const aiNodeAnim *channel);

        struct KeyPosition
        {
            ktm::fvec3 position; // 关键帧位置
            float timeStamp;     // 时间戳
        };
        struct KeyRotation
        {
            ktm::fquat orientation; // 关键帧旋转（四元数）
            float timeStamp;        // 时间戳
        };
        struct KeyScale
        {
            ktm::fvec3 scale; // 关键帧缩放
            float timeStamp;  // 时间戳
        };

        std::vector<KeyPosition> m_Positions; // 所有位置关键帧
        std::vector<KeyRotation> m_Rotations; // 所有旋转关键帧
        std::vector<KeyScale> m_Scales;       // 所有缩放关键帧
        int m_NumPositions;                   // 位置关键帧数量
        int m_NumRotations;                   // 旋转关键帧数量
        int m_NumScales;                      // 缩放关键帧数量
        std::string m_Name;                   // 骨骼名称
        int m_ID;                             // 骨骼ID
    };

    class BoneInfo final : public IResource
    {
      public:
        int ID;                    // 骨骼ID
        std::string Name;          // 骨骼名称
        ktm::fmat4x4 OffsetMatrix; // 绑定姿态到骨骼空间的偏移矩阵
    };

} // namespace Corona
