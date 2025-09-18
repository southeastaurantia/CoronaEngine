#pragma once

#include <Core/Engine/Engine.h>
#include <Core/Engine/Systems/AnimationSystem.h>
#include <Core/Engine/Systems/AudioSystem.h>
#include <Core/Engine/Systems/DisplaySystem.h>
#include <Core/Engine/Systems/RenderingSystem.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include "Resource/Scene.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <Resource/Model.h>
#include <chrono>
#include <filesystem>
#include <thread>
#include <unordered_map>

struct KeyRateLimiter
{
    using clock = std::chrono::steady_clock;
    std::unordered_map<int, clock::time_point> last_time;

    // cooldown_ms: 冷却时间（毫秒）
    bool allow(int key, int cooldown_ms)
    {
        auto now = clock::now();
        auto it = last_time.find(key);
        if (it == last_time.end() || std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count() >= cooldown_ms)
        {
            last_time[key] = now;
            return true;
        }
        return false;
    }

    // 可选：重置某按键计时器
    void reset(int key) { last_time.erase(key); }
};

static KeyRateLimiter key_limiter;
// 不再需要 <vector>，已改为单窗口

inline void Examples4()
{
    // 初始化
    Corona::Engine::Instance().Init({/* LogConfig */});

    // 注册系统
    Corona::Engine::Instance().RegisterSystem<Corona::RenderingSystem>();
    Corona::Engine::Instance().RegisterSystem<Corona::DisplaySystem>();
    Corona::Engine::Instance().RegisterSystem<Corona::AudioSystem>();
    Corona::Engine::Instance().RegisterSystem<Corona::AnimationSystem>();

    // 启动
    Corona::Engine::Instance().StartSystems();

    auto &sceneCache = Corona::Engine::Instance().Cache<Corona::Scene>();
    auto &modelCache = Corona::Engine::Instance().Cache<Corona::Model>();
    auto &animStateCache = Corona::Engine::Instance().Cache<Corona::AnimationState>();
    auto &renderingSystem = Corona::Engine::Instance().GetSystem<Corona::RenderingSystem>();
    auto &animationSystem = Corona::Engine::Instance().GetSystem<Corona::AnimationSystem>();
    auto &render_queue = Corona::Engine::Instance().GetQueue(renderingSystem.name());
    auto &anim_queue = Corona::Engine::Instance().GetQueue(animationSystem.name());

    // 使用数据缓存：加载模型并构建动画状态（若资源存在）
    std::shared_ptr<Corona::Model> model1;
    std::shared_ptr<Corona::Model> model2;
    {
        // 这里假设 ResourceManager 已配置了模型加载器；路径按工程实际
        auto res1 = Corona::Engine::Instance().Resources().load({"model", (std::filesystem::current_path() / "assets/model/armadillo.obj").string()});
        model1 = std::static_pointer_cast<Corona::Model>(res1);
        // std::shared_ptr<Corona::AnimationState> animState;
        if (model1)
        {
            auto modelId = Corona::DataId::Next();
            modelCache.insert(modelId, model1);
            anim_queue.enqueue(&animationSystem, &Corona::AnimationSystem::WatchModel, modelId);
            render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::WatchModel, modelId);
            // if (!model->skeletalAnimations.empty())
            // {
            //     animState = std::make_shared<Corona::AnimationState>();
            //     animState->model = model;
            //     animState->animationIndex = 0;
            //
            //     auto animStateId = Corona::DataId::Next();
            //     animStateCache.insert(animStateId, animState);
            //     anim_queue.enqueue(&animationSystem, &Corona::AnimationSystem::WatchState, animStateId);
            // }
        }
        auto res2 = Corona::Engine::Instance().Resources().load({"model", (std::filesystem::current_path() / "assets/model/Ball.obj").string()});
        model2 = std::static_pointer_cast<Corona::Model>(res2);
        if (model2)
        {
            auto modelId = Corona::DataId::Next();
            modelCache.insert(modelId, model2);
            anim_queue.enqueue(&animationSystem, &Corona::AnimationSystem::WatchModel, modelId);
            render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::WatchModel, modelId);
        }
        CE_LOG_INFO("Model loaded");
    }

    auto shaderCode = Corona::Engine::Instance().Resources().load({"shader", (std::filesystem::current_path() / "assets").string()});
    std::shared_ptr<Corona::Shader> shader = std::static_pointer_cast<Corona::Shader>(shaderCode);
    render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::initShader, shader);


    std::optional<uint64_t> animStateId;

    // 旧的模拟客户端：GLFW 多窗口主循环（无 OpenGL 上下文，便于与 Vulkan/自研渲染对接）
    if (glfwInit() >= 0)
    {
        GLFWwindow *window = nullptr;
        auto scene = std::make_shared<Corona::Scene>();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(800, 800, "Cabbage Engine", nullptr, nullptr);
        if (window)
        {
            auto sceneId = Corona::DataId::Next();
            sceneCache.insert(sceneId, scene);
            scene->displaySurface = glfwGetWin32Window(window);
            render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::WatchScene, sceneId);
            render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::setDisplaySurface, scene);
        }

        auto shouldClosed = [&]() {
            if (!window)
                return true;
            return glfwWindowShouldClose(window) != 0;
        };

        uint64_t frameCount = 0;
        while (!shouldClosed())
        {
            static constexpr uint64_t FPS = 120;
            static constexpr uint64_t TIME = 1000 / FPS;

            auto now = std::chrono::high_resolution_clock::now();

            glfwPollEvents();

            // DO SOMETHING（与系统/渲染交互的占位处）
            {
                // 每 120 帧打印一次动画骨骼数量（若存在）
                if (animStateId && (frameCount % 120 == 0))
                {
                    auto &animCache = Corona::Engine::Instance().Cache<Corona::AnimationState>();
                    auto stConst = animCache.get(*animStateId);
                    if (stConst)
                    {
                        // 注意：const 访问仅用于调试读取
                        const auto bones = stConst->bones.size();
                        (void)bones; // 在没有日志系统的场合避免未使用告警
                    }
                }

                if (window)
                {
                    const int cooldown_ms = 300;
                    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && key_limiter.allow(GLFW_KEY_W, cooldown_ms))
                    {
                        scene->camera.pos.x += 0.5f;
                        CE_LOG_INFO("Key W pressed");
                    }
                    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && key_limiter.allow(GLFW_KEY_S, cooldown_ms))
                    {
                        scene->camera.pos.x -= 0.5f;
                        CE_LOG_INFO("Key S pressed");
                    }
                    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && key_limiter.allow(GLFW_KEY_S, cooldown_ms))
                    {
                        scene->camera.pos.y += 0.5f;
                        CE_LOG_INFO("Key A pressed");
                    }
                    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && key_limiter.allow(GLFW_KEY_S, cooldown_ms))
                    {
                        scene->camera.pos.y -= 0.5f;
                        CE_LOG_INFO("Key D pressed");
                    }
                    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS && key_limiter.allow(GLFW_KEY_ENTER, cooldown_ms))
                    {
                        model1->positon = ktm::fvec3(1.0f, 0.0f, 0.0f);
                        model2->positon = ktm::fvec3(-1.0f, 0.0f, 0.0f);
                        CE_LOG_INFO("Key ENTER pressed");
                    }
                }
                ++frameCount;
            }

            auto end = std::chrono::high_resolution_clock::now();
            if (const auto Spend = std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count();
                Spend < TIME)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(TIME - Spend));
            }
        }
        if (window)
            glfwDestroyWindow(window);
        glfwTerminate();
    }

    // 收尾
    Corona::Engine::Instance().Shutdown();
}