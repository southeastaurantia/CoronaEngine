//
// Created by 47226 on 2025/8/22.
//

#ifndef CABBAGEFRAMEWORK_RENDERINGSYSTEM_H
#define CABBAGEFRAMEWORK_RENDERINGSYSTEM_H

#include <entt/entt.hpp>
#include "CabbageDisplayer.h"
#include <ECS/Events.hpp>

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

    void onSetDisplaySurface(const ECS::Events::SceneSetDisplaySurface& event);
};

#endif // CABBAGEFRAMEWORK_RENDERINGSYSTEM_H
