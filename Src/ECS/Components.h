#pragma once

#include "Pipeline/ComputePipeline.h"
#include "Pipeline/RasterizerPipeline.h"
#include <CabbageDisplayer.h>
#include <entt/entt.hpp>
#include <ktm/ktm.h>
#include <tbb/tbb.h>

namespace ECS::Components
{
    // 资源加载完成标签
    struct ResLoadedTag
    {
    };

    struct Scene
    {
        void *displaySurface;
        HardwareDisplayer displayer;
        HardwareImage finalOutputImage;
    };

    struct Camera
    {
        float fov = 45.0f;
        ktm::fvec3 pos = ktm::fvec3(1.0f, 1.0f, 1.0f);
        ktm::fvec3 forward = ktm::fvec3(-1.0f, -1.0f, -1.0f);
        ktm::fvec3 worldUp = ktm::fvec3(0.0f, 1.0f, 0.0f);
    };

    struct SunLight
    {
        ktm::fvec3 direction{-1.0f, -1.0f, -1.0f};
    };

    struct ActorPose
    {
        ktm::fvec3 transform = ktm::fvec3(0.0f, 0.0f, 0.0f);
        ktm::fvec3 rotate = ktm::fvec3(0.0f, 0.0f, 0.0f);
        ktm::fvec3 scale = ktm::fvec3(1.0f, 1.0f, 1.0f);

        ktm::fmat4x4 getModelMatrix() const
        {
            ktm::fmat4x4 model;
            return model = ktm::translate3d(transform) *
                           ktm::rotate3d_x(rotate.x) * ktm::rotate3d_y(rotate.y) * ktm::rotate3d_z(rotate.z) *
                           ktm::scale3d(scale);
        }
    };

    struct AABB
    {
        ktm::fvec3 aabbMinXYZ = ktm::fvec3(0.0f, 0.0f, 0.0f);
        ktm::fvec3 aabbMaxXYZ = ktm::fvec3(0.0f, 0.0f, 0.0f);
    };

    // Actor的组件
    struct ModelRef
    {
        //uuid model_resource
    };

    // struct ModelResource
    // {
    //     std::string path;
    //     std::vector<Mesh> meshes;
    // }

    struct BoneMatrixHost
    {
        std::vector<ktm::fmat4x4> matrices;
    };

    struct BoneMatrixDevice
    {
        HardwareBuffer matrices;
    };

    struct Meshes
    {
        tbb::concurrent_vector<entt::entity> data;
        std::string path;
    };

    struct MeshHost
    {
        std::vector<uint32_t> indices;
        std::vector<float> positions;
        std::vector<float> normals;
        std::vector<float> texCoords;
        std::vector<uint32_t> boneIndices;
        std::vector<float> boneWeights;
    };

    struct MeshDevice
    {
        HardwareBuffer indicesBuffer;
        HardwareBuffer positionsBuffer;
        HardwareBuffer normalsBuffer;
        HardwareBuffer texCoordsBuffer;
        HardwareBuffer boneIndicesBuffer;
        HardwareBuffer boneWeightsBuffer;
    };

    struct Mesh
    {
        MeshHost host;
        MeshDevice device;
    };

    struct Material
    {
        entt::entity material;
    };

    struct MaterialParams
    {
        ktm::fvec3 baseColor;
        float roughness;
        float metallic;
        float specular;
        float transmission;
    };

    struct BaseColorTexture
    {
        entt::entity texture;
    };

    struct NormalTexture
    {
        entt::entity texture;
    };

    struct OpacityTexture
    {
        entt::entity texture;
    };

    struct ImageHost
    {
        std::string path;            ///< 纹理文件路径
        unsigned char *data;         ///< 纹理数据指针
        int width, height, channels; ///< 纹理宽、高、通道数
    };

    struct ImageDevice
    {
        HardwareImage image;
    };

    struct Model
    {
        entt::entity model;
    };

    struct KeyPosition
    {
        double time;
        ktm::fvec3 position;
    };

    struct KeyRotation
    {
        double time;
        ktm::fquat rotation;
    };

    struct KeyScale
    {
        double time;
        ktm::fvec3 scale;
    };

    struct Bone
    {
        std::vector<KeyPosition> keyPositions;
        std::vector<KeyRotation> keyRotations;
        std::vector<KeyScale> keyScales;
        int NumPositions;
        int NumRotations;
        int NumScales;
        std::string name;
        int id;
    };

    struct AssimpNodeData
    {
        std::string name;
        ktm::fmat4x4 transformation;
        int childrenCount;
        std::vector<AssimpNodeData> children;
    };

    struct SkeletalAnimation
    {
        double duration;
        double ticksPerSecond;
        std::vector<Bone> bones;
        AssimpNodeData rootNode;
    };

    struct BoneInfo
    {
        int id;
        ktm::fmat4x4 offsetMatrix;
    };

    struct Animations
    {
        std::vector<SkeletalAnimation> skeletalAnimations;
        std::map<std::string, BoneInfo> boneInfoMap;
        int boneCount;
    };

} // namespace ECS::Components