
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "CabbageFramework.h"
#include <filesystem>
#include <map>
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
    if (glfwInit() >= 0)
    {
        const std::vector<CabbageFramework::Actor> Actors{
            CabbageFramework::Actor(shaderPath),
            CabbageFramework::Actor(shaderPath)};

        const std::vector<CabbageFramework::Scene> Scenes(4);

        const std::vector<CabbageFramework::Actor> Actors2{
            CabbageFramework::Actor(shaderPath),
            CabbageFramework::Actor(shaderPath)};

        std::vector<GLFWwindow *> windows(4);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        for (size_t i = 0; i < Scenes.size(); i++)
        {
            windows[i] = glfwCreateWindow(800, 800, "Cabbage Engine", nullptr, nullptr);
            Scenes[i].setDisplaySurface(glfwGetWin32Window(windows[i]));

            for (const auto &actor : Actors)
            {
                Scenes[i].addActor(actor);
            }

            for (const auto &actor : Actors2)
            {
                Scenes[i].addActor(actor);
            }

            Scenes[i].setCamera(
                {2.0f, 2.0f, 2.0f},
                {-1.0f, -1.0f, -1.0f},
                {0.0f, 1.0f, 0.0f},
                45.0f);
        }

        auto shouldClosed = [&]() {
            for (const auto & window : windows)
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
        for (const auto & window : windows)
        {
            glfwDestroyWindow(window);
        }
        glfwTerminate();
    }

    return 0;
}