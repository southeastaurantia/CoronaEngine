#include <Python.h>
#include <Script/Python/PythonAPI.h>

inline void Examples3()
{
    Corona::Engine::Instance().Init({/* LogConfig */});

    // 注册系统
    Corona::Engine::Instance().RegisterSystem<Corona::RenderingSystem>();
    Corona::Engine::Instance().RegisterSystem<Corona::DisplaySystem>();
    Corona::Engine::Instance().RegisterSystem<Corona::AudioSystem>();
    Corona::Engine::Instance().RegisterSystem<Corona::AnimationSystem>();

    // 启动
    Corona::Engine::Instance().StartSystems();

    auto &renderingSystem = Corona::Engine::Instance().GetSystem<Corona::RenderingSystem>();
    auto &render_queue = Corona::Engine::Instance().GetQueue(renderingSystem.name());

    auto shaderCode = Corona::Engine::Instance().Resources().load({"shader", (std::filesystem::current_path() / "assets").string()});
    std::shared_ptr<Corona::Shader> shader = std::static_pointer_cast<Corona::Shader>(shaderCode);
    render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::initShader, shader);

    PythonAPI pythonManager;
    std::thread([&]() {
        while (true)
        {
            pythonManager.checkPythonScriptChange();
            pythonManager.checkReleaseScriptChange();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }).detach();

    while (true)
    {
        pythonManager.runPythonScript();
    }

}