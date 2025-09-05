
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "CabbageFramework.h"
#include <filesystem>
#include <regex>
#include <thread>
#include <vector>

std::string shaderPath = [] {
    std::string resultPath = "";
    const std::string runtimePath = std::filesystem::current_path().string();
    // std::replace(runtimePath.begin(), runtimePath.end(), '\\', '/');
    const std::regex pattern(R"((.*)CabbageFramework\b)");
    if (std::smatch matches; std::regex_search(runtimePath, matches, pattern))
    {
        if (matches.size() > 1)
        {
            resultPath = matches[1].str() + "CabbageFramework";
        }
        else
        {
            throw std::runtime_error("Failed to resolve source path.");
        }
    }
    std::ranges::replace(resultPath, '\\', '/');
    return resultPath + "/Examples/assest/model/dancing_vampire.dae";
}();

int main()
{
    CoronaEngine::AnimationSystemDefault::get_singleton().start();
    CoronaEngine::AudioSystemDefault::get_singleton().start();
    CoronaEngine::RenderingSystemDefault::get_singleton().start();
    CoronaEngine::DisplaySystemDefault::get_singleton().start();

    if (glfwInit() >= 0)
    {
        // const std::vector<CoronaEngine::Actor> Actors{
        //     CoronaEngine::Actor(shaderPath),
        //     CoronaEngine::Actor(shaderPath)};
        //
        // const std::vector<CoronaEngine::Scene> Scenes(4);
        //
        // const std::vector<CoronaEngine::Actor> Actors2{
        //     CoronaEngine::Actor(shaderPath),
        //     CoronaEngine::Actor(shaderPath)};

        std::vector<GLFWwindow *> windows(4);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        for (size_t i = 0; i < windows.size(); i++)
        {
            windows[i] = glfwCreateWindow(800, 800, "Cabbage Engine", nullptr, nullptr);
            // Scenes[i].setDisplaySurface(glfwGetWin32Window(windows[i]));


            // Scenes[i].setCamera(
            //     {2.0f, 2.0f, 2.0f},
            //     {-1.0f, -1.0f, -1.0f},
            //     {0.0f, 1.0f, 0.0f},
            //     45.0f);
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

        while (!shouldClosed())
        {
            static constexpr uint64_t FPS = 120;
            static constexpr uint64_t TIME = 1000 / FPS;

            auto now = std::chrono::high_resolution_clock::now();

            glfwPollEvents();

            // DO SOMETHING
            {
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

    CoronaEngine::AnimationSystemDefault::get_singleton().stop();
    CoronaEngine::AudioSystemDefault::get_singleton().stop();
    CoronaEngine::RenderingSystemDefault::get_singleton().stop();
    CoronaEngine::DisplaySystemDefault::get_singleton().stop();

    return 0;
}