#pragma once

#include <CabbageDisplayer.h>
#include <entt/entt.hpp>
#include <ktm/ktm.h>

namespace ECS::Components
{
    struct ResLoadedTag{};

    struct ActorPose
    {
        ktm::fvec3 transform = ktm::fvec3(0.0f, 0.0f, 0.0f);
        ktm::fvec3 rotate = ktm::fvec3(0.0f, 0.0f, 0.0f);
        ktm::fvec3 scale = ktm::fvec3(1.0f, 1.0f, 1.0f);

        ktm::fvec3 aabbMinXYZ = ktm::fvec3(0.0f, 0.0f, 0.0f);
        ktm::fvec3 aabbMaxXYZ = ktm::fvec3(0.0f, 0.0f, 0.0f);
    };

    struct BoneMatrixHost
    {
        std::vector<ktm::fmat4x4> matrices;
    };

    struct BoneMatrixDevice
    {
        HardwareBuffer matrices;
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

    struct MaterialParams
    {
        ktm::fvec3 baseColor;
        float roughness;
        float metallic;
        float specular;
        float transmission;
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

    struct Material
    {
        entt::entity material;
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

    struct Meshes
    {
        std::vector<entt::entity> meshes;
        std::string path;
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
        ktm::fvec3 direction;
    };

    struct Actors
    {
        std::vector<entt::entity> actors;
    };

    struct Model
    {
        entt::entity model;
    };
} // namespace ECS::Components