#pragma once

#include "ECS/Components.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <entt/entt.hpp>
#include <string>

namespace ECS
{
    class ResourceManager final
    {
      public:
        ResourceManager();
        ~ResourceManager();

        // TODO: Implement resource management functions
        entt::entity LoadModel(const std::string &resourcePath);
        void ProcessNode(aiNode *node, const aiScene *scene, entt::entity parentEntity);
        entt::entity ProcessMesh(aiMesh *mesh, const aiScene *scene, entt::entity modelEntity);
        void ExtractBoneWeightForVertices(aiMesh *mesh, Components::MeshHost &meshHost, const aiScene *scene, entt::entity modelEntity);

      private:
    };
} // namespace ECS