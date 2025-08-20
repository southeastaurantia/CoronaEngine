
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "CabbageFramework.h"
#include <vector>
#include <regex>
#include <filesystem>
#include <map>

std::string shaderPath = [] {
    std::string resultPath = "";
    std::string runtimePath = std::filesystem::current_path().string();
    // std::replace(runtimePath.begin(), runtimePath.end(), '\\', '/');
    std::regex pattern(R"((.*)CabbageFramework\b)");
    std::smatch matches;
    if (std::regex_search(runtimePath, matches, pattern))
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
    std::replace(resultPath.begin(), resultPath.end(), '\\', '/');
    return resultPath + "/Examples";
}();

int main()
{
    std::vector<CabbageFramework::Scene> scenes;
    std::map<CabbageFramework::Scene, std::vector<CabbageFramework::Actor>> actors;



    if (glfwInit() >= 0)
    {
        scenes.resize(4);
        std::vector<GLFWwindow *> windows(4);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        for (size_t i = 0; i < scenes.size(); i++)
        {
            windows[i] = glfwCreateWindow(800, 800, "Cabbage Engine", nullptr, nullptr);
            scenes[i].setDisplaySurface(glfwGetWin32Window(windows[i]));
            actors[scenes[i]].emplace_back(scenes[i], shaderPath);
        }

        auto shouldClosed = [&]() {
            for (size_t i = 0; i < windows.size(); i++)
            {
                if (glfwWindowShouldClose(windows[i]))
                {
                    return true;
                }
            }
            return false;
        };

        while (!shouldClosed())
        {
            glfwPollEvents();
        }
        for (size_t i = 0; i < windows.size(); i++)
        {
            glfwDestroyWindow(windows[i]);
        }
        glfwTerminate();
    }

    return 0;
}