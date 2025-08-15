#pragma once
#include <entt/entt.hpp>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "ECS/Components.h"


namespace ECS
{
    class ResourceManager final
    {
      public:
        ResourceManager(entt::registry& registry);
        ~ResourceManager();

        // TODO: Implement resource management functions
        entt::entity LoadModel(const std::string &resourcePath);
        void ProcessNode(aiNode* node, const aiScene* scene, entt::entity parentEntity);
        entt::entity ProcessMesh(aiMesh* mesh, const aiScene* scene, entt::entity modelEntity);
        void ExtractBoneWeightForVertices(aiMesh* mesh, Components::MeshHost& meshHost, const aiScene* scene, entt::entity modelEntity);

      private:
        entt::registry& m_registry;
    };
} // namespace ECS