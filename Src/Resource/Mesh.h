#pragma once
#include "CabbageDisplayer.h"
#include "Core/IO/IResource.h"

#include <assimp/material.h>
#include <ktm/type_vec.h>
#include <limits>

namespace Corona
{

    class Texture final : public IResource
    {
      public:
        aiTextureType type;            ///< 纹理类型 (aiTextureType)
        std::string path;              ///< 纹理文件路径
        unsigned char *data;           ///< 纹理数据指针
        int width, height, nrChannels; ///< 纹理宽、高、通道数
    };

    class MeshDevice final : public IResource
    {
      public:
        HardwareBuffer pointsBuffer;        ///< 顶点坐标缓冲区
        HardwareBuffer normalsBuffer;       ///< 法线缓冲区
        HardwareBuffer texCoordsBuffer;     ///< 纹理坐标缓冲区
        HardwareBuffer indexBuffer;         ///< 索引缓冲区
        HardwareBuffer boneIndexesBuffer;   ///< 骨骼索引缓冲区
        HardwareBuffer boneWeightsBuffer;   ///< 骨骼权重缓冲区

        // 默认使用无效索引，避免未初始化使用
        uint32_t materialIndex = std::numeric_limits<uint32_t>::max();
        uint32_t textureIndex = std::numeric_limits<uint32_t>::max();
    };

    class Mesh final : public IResource
    {
      public:
        ktm::fvec3 minXYZ = ktm::fvec3(0.0f, 0.0f, 0.0f); ///< 包围盒最小点
        ktm::fvec3 maxXYZ = ktm::fvec3(0.0f, 0.0f, 0.0f); ///< 包围盒最大点
        std::vector<uint32_t> Indices;        ///< 三角面片顶点索引
        std::vector<float> points;                        ///< 顶点坐标（始终按索引存储）
        std::vector<float> normals;                       ///< 法线（按顶点或插值方式存储）
        std::vector<float> texCoords;                     ///< 纹理坐标（按顶点或插值方式存储）
        std::vector<uint32_t> boneIndices;                ///< 骨骼索引（每个顶点最多4个骨骼）
        std::vector<float> boneWeights;                   ///< 骨骼权重（与boneIndices一一对应）
        std::vector<std::shared_ptr<Texture>> textures;   ///< 网格使用的所有纹理
        std::shared_ptr<MeshDevice> meshDevice;          ///< 网格的设备相关数据
    };

} // namespace Corona
