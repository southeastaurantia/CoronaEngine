#pragma once

#include "Animation.h"
#include "Bone.h"
#include "Core/IO/IResource.h"
#include "Core/IO/IResourceLoader.h"
#include "Core/IO/ResourceTypes.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// forward decls for Assimp types used in declarations
struct aiMesh;
struct aiScene;
struct aiNode;
struct aiMaterial;
#include "Mesh.h"

namespace Corona
{
    class Model final : public IResource
    {
      public:
        std::vector<Mesh> meshes;                                       ///< 所有网格
        std::map<std::string, std::shared_ptr<BoneInfo>> m_BoneInfoMap; ///< 骨骼信息映射表
        int m_BoneCounter = 0;                                          ///< 骨骼计数器
        std::vector<Animation> skeletalAnimations;                      ///< 所有骨骼动画
        ktm::fvec3 minXYZ = ktm::fvec3(0.0f, 0.0f, 0.0f);               ///< 模型包围盒最小点
        ktm::fvec3 maxXYZ = ktm::fvec3(0.0f, 0.0f, 0.0f);               ///< 模型包围盒最大点
        ktm::fvec3 positon = ktm::fvec3(0.0f, 0.0f, 0.0f);                           ///< 位置
        ktm::fvec3 rotation = ktm::fvec3(0.0f, 0.0f, 0.0f);                          ///< 旋转
        ktm::fvec3 scale = ktm::fvec3(1.0f, 1.0f, 1.0f);                            ///< 缩放
        ktm::fmat4x4 modelMatrix = ktm::fmat4x4::from_diag(ktm::fvec4(0.0, 0.0, 0.0, 0.0));                          ///< 模型矩阵
        HardwareBuffer bonesMatrixBuffer;
    };

    class ModelLoader : public IResourceLoader
    {
      public:
        // 支持类型：type=="model" 或 扩展名常见三维格式
        bool supports(const ResourceId &id) const override;
        std::shared_ptr<IResource> load(const ResourceId &id) override;

      private:
        using ModelPtr = std::shared_ptr<Model>;
        void processMesh(const std::string &path, const aiMesh *mesh, const aiScene *scene, const ModelPtr &model, Mesh &resultMesh);
        void processNode(const std::string &path, const aiNode *node, const aiScene *scene, const ModelPtr &model);
        void loadMaterial(const std::string &path, const aiMaterial *material, Mesh &resultMesh);
        void extractBoneWeightForVertices(Mesh &resultMesh, const aiMesh *mesh, const aiScene *scene, const ModelPtr &model);
        std::unordered_map<std::string, std::shared_ptr<Texture>> texturePathHash;
        std::unordered_map<std::string, HardwareImage> textureImageHash;
        uint32_t attributeToImageIndex = 0;
    };
} // namespace Corona
