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