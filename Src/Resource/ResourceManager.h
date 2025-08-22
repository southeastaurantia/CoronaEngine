#pragma once

#include "ECS/Components.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <entt/entt.hpp>
#include <fstream>
#include <string>

namespace ECS
{
    class ResourceManager final
    {
      public:
        explicit ResourceManager(std::shared_ptr<entt::registry> registry);
        ~ResourceManager();

        // TODO: Implement resource management functions
        entt::entity LoadModel(const std::string &resourcePath);
        void ProcessNode(aiNode *node, const aiScene *scene, entt::entity parentEntity);
        entt::entity ProcessMesh(aiMesh *mesh, const aiScene *scene, entt::entity modelEntity);
        void ExtractBoneWeightForVertices(aiMesh *mesh, Components::MeshHost &meshHost, const aiScene *scene, entt::entity modelEntity);
        std::string loadShader(const std::string &shaderPath);
        void loadDemo(const std::string &demoPath, entt::entity modelEntity);
        void createMesh(entt::entity modelEntity);
        void createComputeUniformBuffer(entt::entity modelEntity);

      private:
        std::shared_ptr<entt::registry> registry;
    };
} // namespace ECS