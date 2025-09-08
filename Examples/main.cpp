
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "CabbageFramework.h"
#include "Engine.h"
#include "spirv_cross.hpp"

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

#include <Resource/ResourceManager.h>
#include <iostream>

class Texture final : public Corona::Resource
{
  public:
    int width;
    int height;
};

class TextureLoader final : public Corona::ResourceLoader<Texture>
{
  public:
    bool on_load(const std::string &path, ResourceHandle resource) override
    {
        resource->width = 10;
        resource->height = 20;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        return true;
    }
};

#include <Core/Logger.h>

int main()
{
    Corona::Engine::inst().init();

    LOG_INFO("Application starting with decoupled logger.");
    LOG_WARN("This is a test warning: {}.", "some value");
    LOG_ERROR("File not found at '{}'", "/path/to/file.txt");

    std::cin.get();

    // if (glfwInit() >= 0)
    // {
    //     // const std::vector<CoronaEngine::Actor> Actors{
    //     //     CoronaEngine::Actor(shaderPath),
    //     //     CoronaEngine::Actor(shaderPath)};
    //     //
    //     // const std::vector<CoronaEngine::Scene> Scenes(4);
    //     //
    //     // const std::vector<CoronaEngine::Actor> Actors2{
    //     //     CoronaEngine::Actor(shaderPath),
    //     //     CoronaEngine::Actor(shaderPath)};
    //
    //     std::vector<GLFWwindow *> windows(4);
    //
    //     glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //     for (size_t i = 0; i < windows.size(); i++)
    //     {
    //         windows[i] = glfwCreateWindow(800, 800, "Cabbage Engine", nullptr, nullptr);
    //         // Scenes[i].setDisplaySurface(glfwGetWin32Window(windows[i]));
    //
    //         // Scenes[i].setCamera(
    //         //     {2.0f, 2.0f, 2.0f},
    //         //     {-1.0f, -1.0f, -1.0f},
    //         //     {0.0f, 1.0f, 0.0f},
    //         //     45.0f);
    //     }
    //
    //     auto shouldClosed = [&]() {
    //         for (const auto &window : windows)
    //         {
    //             if (glfwWindowShouldClose(window))
    //             {
    //                 return true;
    //             }
    //         }
    //         return false;
    //     };
    //
    //     while (!shouldClosed())
    //     {
    //         static constexpr uint64_t FPS = 120;
    //         static constexpr uint64_t TIME = 1000 / FPS;
    //
    //         auto now = std::chrono::high_resolution_clock::now();
    //
    //         glfwPollEvents();
    //
    //         // DO SOMETHING
    //         {
    //             CoronaEngine::AnimationSystemDefault::get_singleton().tick();
    //             CoronaEngine::AudioSystemDefault::get_singleton().tick();
    //             CoronaEngine::RenderingSystemDefault::get_singleton().tick();
    //             CoronaEngine::DisplaySystemDefault::get_singleton().tick();
    //         }
    //
    //         auto end = std::chrono::high_resolution_clock::now();
    //         if (const auto Spend = std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count();
    //             Spend < TIME)
    //         {
    //             std::this_thread::sleep_for(std::chrono::milliseconds(TIME - Spend));
    //         }
    //     }
    //     for (const auto &window : windows)
    //     {
    //         glfwDestroyWindow(window);
    //     }
    //     glfwTerminate();
    // }

    return 0;
}