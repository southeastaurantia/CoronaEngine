
#include <filesystem>
#include <iostream>
#include <thread>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "CabbageFramework.h"
#include "ECS/Global.h"

#include <functional>


int main()
{
    auto& sceneMgr = ECS::Global::get().sceneMgr;
    std::vector<entt::entity> sceneIds;

    if (glfwInit() >= 0)
    {
        sceneIds.resize(4);
        std::vector<GLFWwindow *> windows(4);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        for (size_t i = 0; i < 4; i++)
        {
            windows[i] = glfwCreateWindow(800, 800, "Cabbage Engine", nullptr, nullptr);
            entt::entity sceneId = sceneMgr->createScene(glfwGetWin32Window(windows[i]), false);
            sceneIds[i] = sceneId;
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