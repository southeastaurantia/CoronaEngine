//
// Created by 47226 on 2025/8/22.
//

#ifndef CABBAGEFRAMEWORK_RENDERINGSYSTEM_H
#define CABBAGEFRAMEWORK_RENDERINGSYSTEM_H

#include "CabbageDisplayer.h"
#include <ECS/Events.hpp>
#include <entt/entt.hpp>

#include <thread>

class RenderingSystem
{
  public:
    static constexpr int RenderFPS = 120;
    static constexpr int DisplayFPS = 240;
    static constexpr float RenderMinFrameTime = 1.0f / RenderFPS;
    static constexpr float DisplayMinFrameTime = 1.0f / DisplayFPS;

    explicit RenderingSystem(std::shared_ptr<entt::registry> registry);
    ~RenderingSystem();

  private:
    void renderLoop();
    void displayLoop();

    bool running;
    std::thread renderThread;
    std::thread displayThread;
    std::shared_ptr<entt::registry> registry;
    HardwareDisplayer displayer;

    void onSetDisplaySurface(ECS::Events::SceneSetDisplaySurface event);
    void updateEngine(entt::entity scene);
};

#endif // CABBAGEFRAMEWORK_RENDERINGSYSTEM_H
