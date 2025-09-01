//
// Created by 47226 on 2025/8/22.
//

#ifndef CABBAGEFRAMEWORK_ANIMATIONSYSTEM_H
#define CABBAGEFRAMEWORK_ANIMATIONSYSTEM_H

#include <entt/entt.hpp>

#include <thread>

class AnimationSystem
{
  public:
    static constexpr int FPS = 120;
    static constexpr float MinFrameTime = 1.0f / FPS;

    explicit AnimationSystem(const std::shared_ptr<entt::registry> &registry);
    ~AnimationSystem();

    void start();
    void stop();

  private:
    void loop();

    std::atomic<bool> running;
    std::thread loopThread;
    std::shared_ptr<entt::registry> registry;
};

#endif // CABBAGEFRAMEWORK_ANIMATIONSYSTEM_H
