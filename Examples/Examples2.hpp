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

#include <Resource/Mesh.h>
#include <Resource/Model.h>
#include <chrono>
#include <filesystem>
#include <thread>
#include <vector>

inline void Examples2()
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
    auto &renderingSystem = Corona::Engine::Instance().GetSystem<Corona::RenderingSystem>();
    auto &render_queue = Corona::Engine::Instance().GetQueue(renderingSystem.name());

    // 使用数据缓存：加载模型并构建动画状态（若资源存在）
    std::shared_ptr<Corona::Model> model;
    {
        // 这里假设 ResourceManager 已配置了模型加载器；路径按工程实际
        auto res = Corona::Engine::Instance().Resources().load({"model", (std::filesystem::current_path() / "assets/model/dancing_vampire.dae").string()});
        model = std::static_pointer_cast<Corona::Model>(res);
        CE_LOG_INFO("Model loaded");
    }

    auto shaderCode = Corona::Engine::Instance().Resources().load({"shader", (std::filesystem::current_path() / "assets").string()});
    std::shared_ptr<Corona::Shader> shader = std::static_pointer_cast<Corona::Shader>(shaderCode);
    render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::initShader, shader);

    if (model)
    {
        auto modelId = Corona::DataId::Next();
        modelCache.insert(modelId, model);
        render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::WatchModel, modelId);
    }

    std::optional<uint64_t> animStateId;

    // 旧的模拟客户端：GLFW 多窗口主循环（无 OpenGL 上下文，便于与 Vulkan/自研渲染对接）
    if (glfwInit() >= 0)
    {
        std::vector<GLFWwindow *> windows(4);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        for (size_t i = 0; i < windows.size(); i++)
        {
            windows[i] = glfwCreateWindow(800, 800, "Cabbage Engine", nullptr, nullptr);
            auto scene = std::make_shared<Corona::Scene>();
            auto sceneId = Corona::DataId::Next();
            sceneCache.insert(sceneId, scene);
            scene->displaySurface = glfwGetWin32Window(windows[i]);
            render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::WatchScene, sceneId);
            render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::setDisplaySurface, scene);
        }

        auto shouldClosed = [&]() {
            for (const auto &window : windows)
            {
                if (glfwWindowShouldClose(window))
                {
                    return true;
                }
            }
            return false;
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

                // 简易键盘控制：数字键 1/2 观察/取消观察 meshId
                // 注意：GLFW 需窗口上下文，这里取第一个窗口
                // if (!windows.empty())
                // {
                //     auto *w = windows[0];
                //     if (glfwGetKey(w, GLFW_KEY_1) == GLFW_PRESS)
                //     {
                //         Corona::RenderingSystem::WatchMesh(meshId);
                //     }
                //     if (glfwGetKey(w, GLFW_KEY_2) == GLFW_PRESS)
                //     {
                //         Corona::RenderingSystem::UnwatchMesh(meshId);
                //     }
                // }
                ++frameCount;
            }

            auto end = std::chrono::high_resolution_clock::now();
            if (const auto Spend = std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count();
                Spend < TIME)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(TIME - Spend));
            }
        }
        for (const auto &window : windows)
        {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }

    // 收尾
    Corona::Engine::Instance().Shutdown();
}