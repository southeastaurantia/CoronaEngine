//
// Created by 25473 on 25-9-9.
//

#pragma once
#include "CabbageDisplayer.h"
#include "Core/IO/Resource.h"
#include "assimp/material.h"
#include "ktm/type_vec.h"

namespace Corona {

    class Texture final : public Resource
    {
    public:
        aiTextureType type;            ///< 纹理类型
        std::string path;              ///< 纹理文件路径
        unsigned char *data;           ///< 纹理数据指针
        int width, height, nrChannels; ///< 纹理宽、高、通道数
    };

    class Mesh final : public Resource {
        public:
            ktm::fvec3 minXYZ = ktm::fvec3(0.0f, 0.0f, 0.0f); ///< 包围盒最小点
            ktm::fvec3 maxXYZ = ktm::fvec3(0.0f, 0.0f, 0.0f); ///< 包围盒最大点
            std::vector<uint32_t> triangulatedIndices;        ///< 三角面片顶点索引
            std::vector<float> points;                        ///< 顶点坐标（始终按索引存储）
            std::vector<float> normals;                       ///< 法线（按顶点或插值方式存储）
            std::vector<float> texCoords;                     ///< 纹理坐标（按顶点或插值方式存储）
            std::vector<uint32_t> boneIndices;                ///< 骨骼索引（每个顶点最多4个骨骼）
            std::vector<float> boneWeights;                   ///< 骨骼权重（与boneIndices一一对应）
            std::vector<std::shared_ptr<Texture>> textures;                    ///< 网格使用的所有纹理
    };

} // Corona

