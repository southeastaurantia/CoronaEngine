// //
// // Created by 47226 on 2025/8/22.
// //
//
// #ifndef CABBAGEFRAMEWORK_RENDERINGSYSTEM_H
// #define CABBAGEFRAMEWORK_RENDERINGSYSTEM_H
//
// #include <Resource/ResourceManager.h>
// #include "CabbageDisplayer.h"
// #include "PipeLine/RasterizerPipeline.h"
// #include "PipeLine/ComputePipeline.h"
// #include <ECS/Events.hpp>
// #include <ECS/Components.h>
// #include <entt/entt.hpp>
// #include <memory>
// #include <ktm/ktm.h>
//
// #include <thread>
//
// class RenderingSystem
// {
//   public:
//     static constexpr int RenderFPS = 120;
//     static constexpr int DisplayFPS = 240;
//     static constexpr float RenderMinFrameTime = 1.0f / RenderFPS;
//     static constexpr float DisplayMinFrameTime = 1.0f / DisplayFPS;
//
//     explicit RenderingSystem(const std::shared_ptr<entt::registry> &registry);
//     ~RenderingSystem();
//
//     void start();
//     void stop();
//
//   private:
//     void renderLoop();
//     void displayLoop();
//
//     std::atomic<bool> running;
//     std::thread renderThread;
//     std::thread displayThread;
//     std::shared_ptr<entt::registry> registry;
//
//     ktm::uvec2 gbufferSize = ktm::uvec2(800, 800);
//     HardwareImage gbufferPostionImage;
//     HardwareImage gbufferBaseColorImage;
//     HardwareImage gbufferNormalImage;
//     HardwareImage gbufferMotionVectorImage;
//
//     HardwareBuffer uniformBuffer;
//     HardwareBuffer gbufferUniformBuffer;
//
//     std::string shaderPath = ECS::ResourceManager::getShaderPath();
//
//     //RasterizerPipeline gbufferPipelineObj = RasterizerPipeline(ECS::ResourceManager::readStringFile(shaderPath + "/shaders/test.vert.glsl"), ECS::ResourceManager::readStringFile(shaderPath + "/shaders/test.frag.glsl"));
//     //ComputePipeline compositePipelineObj = ComputePipeline(ECS::ResourceManager::readStringFile(shaderPath + "/shaders/test.comp.glsl"));
//
//     struct UniformBufferObject
//     {
//         ktm::fvec3 lightPostion;
//         ktm::fmat4x4 lightViewMatrix;
//         ktm::fmat4x4 lightProjMatrix;
//
//         ktm::fvec3 eyePosition;
//         ktm::fvec3 eyeDir;
//         ktm::fmat4x4 eyeViewMatrix;
//         ktm::fmat4x4 eyeProjMatrix;
//     } uniformBufferObjects;
//
//     struct gbufferUniformBufferObject
//     {
//         ktm::fmat4x4 viewProjMatrix;
//     } gbufferUniformBufferObjects;
//
//
//     //HardwareBuffer sceneUniformBuffer;
//     //HardwareBuffer gbufferUniformBuffer;
//
//     void onSetDisplaySurface(ECS::Events::SceneSetDisplaySurface event);
//     void updateEngine(entt::entity scene);
//     void gbufferPipeline(entt::entity scene);
//     void compositePipeline(ECS::Components::Scene scene, ktm::fvec3 sunDir = ktm::fvec3(0.0, 1.0, 0.0));
// };
//
// #endif // CABBAGEFRAMEWORK_RENDERINGSYSTEM_H
