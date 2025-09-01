//
// Created by 47226 on 2025/8/22.
//

#ifndef CABBAGEFRAMEWORK_AUDIOSYSTEM_H
#define CABBAGEFRAMEWORK_AUDIOSYSTEM_H

#include <entt/entt.hpp>

#include <thread>

class AudioSystem
{
  public:
    static constexpr int FPS = 120;
    static constexpr float MinFrameTime = 1.0f / FPS;

    explicit AudioSystem(const std::shared_ptr<entt::registry> &registry);
    ~AudioSystem();

    void start();
    void stop();

  private:
    void loop();

    std::atomic<bool> running;
    std::thread loopThread;
    std::shared_ptr<entt::registry> registry;
};

#endif // CABBAGEFRAMEWORK_AUDIOSYSTEM_H
