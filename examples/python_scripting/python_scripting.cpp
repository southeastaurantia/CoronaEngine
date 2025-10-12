#include <Python.h>
#include <script/python/PythonAPI.h>
#include <core/engine/Engine.h>
#include <core/engine/systems/animation/AnimationSystem.h>
#include <core/engine/systems/audio/AudioSystem.h>
#include <core/engine/systems/display/DisplaySystem.h>
#include <core/engine/systems/rendering/RenderingSystem.h>
#include <resource/Shader.h>
#include <chrono>
#include <thread>

int main()
{
    Corona::Engine::instance().init({/* LogConfig */});

    // 注册系统
    Corona::Engine::instance().register_system<Corona::RenderingSystem>();
    Corona::Engine::instance().register_system<Corona::DisplaySystem>();
    Corona::Engine::instance().register_system<Corona::AudioSystem>();
    Corona::Engine::instance().register_system<Corona::AnimationSystem>();

    // 启动
    Corona::Engine::instance().start_systems();

    auto &rendering_system = Corona::Engine::instance().get_system<Corona::RenderingSystem>();
    auto &render_queue = Corona::Engine::instance().get_queue(rendering_system.name());

    auto shader_code = Corona::Engine::instance().resources().load({"shader", (std::filesystem::current_path() / "assets").string()});
    std::shared_ptr<Corona::Shader> shader = std::static_pointer_cast<Corona::Shader>(shader_code);
    render_queue.enqueue(&rendering_system, &Corona::RenderingSystem::init_shader, shader);

    PythonAPI pythonManager;
    std::thread([&]() {
        while (true)
        {
            // pythonManager.checkPythonScriptChange();
            pythonManager.checkReleaseScriptChange();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }).detach();

    while (true)
    {
        pythonManager.runPythonScript();
    }

    return 0;
}