//
// Created by 25473 on 25-9-9.
//

#ifndef MODEL_H
#define MODEL_H

#include "Animation.h"
#include "Bone.h"
#include "Core/IO/Resource.h"
#include "Core/IO/ResourceLoader.h"
#include "Mesh.h"
#include "assimp/scene.h"

#include <map>
#include <vector>

namespace Corona
{
    class Model final : public Resource {
        public:
            std::vector<Mesh> meshes;                                 ///< 所有网格
            std::map<std::string, std::shared_ptr<BoneInfo>> m_BoneInfoMap;            ///< 骨骼信息映射表
            int m_BoneCounter = 0;                                    ///< 骨骼计数器
            std::vector<Animation> skeletalAnimations;                ///< 所有骨骼动画
            ktm::fvec3 minXYZ = ktm::fvec3(0.0f, 0.0f, 0.0f);         ///< 模型包围盒最小点
            ktm::fvec3 maxXYZ = ktm::fvec3(0.0f, 0.0f, 0.0f);         ///< 模型包围盒最大点
    };

    class ModelLoader : public ResourceLoader<Model>
    {
        public:
            using Handle = ResourceLoader<Model>::Handle;
            bool load(const std::string &path, Handle &handle) override;

        private:
            void processMesh(const std::string& path, const aiMesh* mesh, const aiScene* scene, Handle &handle, Mesh &resultMesh);
            void processNode(const std::string& path, const aiNode* node, const aiScene* scene, Handle &handle);
            void loadMaterial(const std::string& path, const aiMaterial* material, Mesh& resultMesh);
            void extractBoneWeightForVertices(Mesh& resultMesh, const aiMesh* mesh, const aiScene* scene, Handle &handle);
            std::unordered_map<std::string, std::shared_ptr<Texture>> texturePathHash;
            uint32_t attributeToImageIndex = 0;                       ///< 属性转图片的索引
    };
}// Corona

#endif //MODEL_H
