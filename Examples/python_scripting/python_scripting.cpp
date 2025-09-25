#include <Python.h>
#include <Script/Python/PythonAPI.h>
#include <Core/Engine/Engine.h>
#include <Core/Engine/Systems/AnimationSystem.h>
#include <Core/Engine/Systems/AudioSystem.h>
#include <Core/Engine/Systems/DisplaySystem.h>
#include <Core/Engine/Systems/RenderingSystem.h>
#include <chrono>
#include <thread>

int main()
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

    return 0;
}