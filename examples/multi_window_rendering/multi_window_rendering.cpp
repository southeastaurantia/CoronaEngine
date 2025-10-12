#include <core/engine/Engine.h>
#include <core/engine/systems/animation/AnimationSystem.h>
#include <core/engine/systems/audio/AudioSystem.h>
#include <core/engine/systems/display/DisplaySystem.h>
#include <core/engine/systems/rendering/RenderingSystem.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include "resource/Scene.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <resource/Mesh.h>
#include <resource/Model.h>
#include <resource/Shader.h>
#include <chrono>
#include <filesystem>
#include <optional>
#include <thread>
#include <vector>

int main()
{
    // 初始化
    auto &engine = Corona::Engine::instance();
    engine.init({/* LogConfig */});

    // 注册系统
    engine.register_system<Corona::RenderingSystem>();
    engine.register_system<Corona::DisplaySystem>();
    engine.register_system<Corona::AudioSystem>();
    engine.register_system<Corona::AnimationSystem>();

    // 启动
    engine.start_systems();

    auto &scene_cache = engine.cache<Corona::Scene>();
    auto &model_cache = engine.cache<Corona::Model>();
    auto &anim_state_cache = engine.cache<Corona::AnimationState>();
    auto &rendering_system = engine.get_system<Corona::RenderingSystem>();
    auto &animation_system = engine.get_system<Corona::AnimationSystem>();
    auto &render_queue = engine.get_queue(rendering_system.name());
    auto &anim_queue = engine.get_queue(animation_system.name());

    std::optional<uint64_t> anim_state_id;

    // 使用数据缓存：加载模型并构建动画状态（若资源存在）
    std::shared_ptr<Corona::Model> model;
    {
        // 这里假设 ResourceManager 已配置了模型加载器；路径按工程实际
        auto res = engine.resources().load({"model", (std::filesystem::current_path() / "assets/model/dancing_vampire.dae").string()});
        model = std::static_pointer_cast<Corona::Model>(res);
        std::shared_ptr<Corona::AnimationState> anim_state;
        if (model)
        {
            if (!model->skeletalAnimations.empty())
            {
                anim_state = std::make_shared<Corona::AnimationState>();
                anim_state->model = model;
                anim_state->animationIndex = 0;

                const auto new_anim_state_id = Corona::DataId::next();
                anim_state_id = new_anim_state_id;
                anim_state_cache.insert(new_anim_state_id, anim_state);
                anim_queue.enqueue(&animation_system, &Corona::AnimationSystem::watch_state, new_anim_state_id);
            }
        }
        CE_LOG_INFO("Model loaded");
    }

    auto shader_code = engine.resources().load({"shader", (std::filesystem::current_path() / "assets").string()});
    std::shared_ptr<Corona::Shader> shader = std::static_pointer_cast<Corona::Shader>(shader_code);
    render_queue.enqueue(&rendering_system, &Corona::RenderingSystem::init_shader, shader);

    if (model)
    {
        const auto model_id = Corona::DataId::next();
        model_cache.insert(model_id, model);
    anim_queue.enqueue(&animation_system, &Corona::AnimationSystem::watch_model, model_id);
    render_queue.enqueue(&rendering_system, &Corona::RenderingSystem::watch_model, model_id);
    }

    // 旧的模拟客户端：GLFW 多窗口主循环（无 OpenGL 上下文，便于与 Vulkan/自研渲染对接）
    if (glfwInit() >= 0)
    {
        std::vector<GLFWwindow *> windows(4);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        for (size_t i = 0; i < windows.size(); i++)
        {
            windows[i] = glfwCreateWindow(800, 800, "Cabbage Engine", nullptr, nullptr);
            auto scene = std::make_shared<Corona::Scene>();
            const auto scene_id = Corona::DataId::next();
            scene_cache.insert(scene_id, scene);
            scene->displaySurface = glfwGetWin32Window(windows[i]);
            render_queue.enqueue(&rendering_system, &Corona::RenderingSystem::watch_scene, scene_id);
            render_queue.enqueue(&rendering_system, &Corona::RenderingSystem::set_display_surface, scene);
        }

        auto should_closed = [&]() {
            for (const auto &window : windows)
            {
                if (glfwWindowShouldClose(window))
                {
                    return true;
                }
            }
            return false;
        };

        uint64_t frame_count = 0;
        while (!should_closed())
        {
            static constexpr uint64_t FPS = 120;
            static constexpr uint64_t TIME = 1000 / FPS;

            auto now = std::chrono::high_resolution_clock::now();

            glfwPollEvents();

            // DO SOMETHING（与系统/渲染交互的占位处）
            {
                // 每 120 帧打印一次动画骨骼数量（若存在）
                if (anim_state_id && (frame_count % 120 == 0))
                {
                    auto &anim_cache = engine.cache<Corona::AnimationState>();
                    auto state_const = anim_cache.get(*anim_state_id);
                    if (state_const)
                    {
                        // 注意：const 访问仅用于调试读取
                        const auto bones = state_const->bones.size();
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
                ++frame_count;
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
    engine.shutdown();
    return 0;
}