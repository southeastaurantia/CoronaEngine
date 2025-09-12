#include "Core/Engine.h"
#include "Core/IO/ResourceManager.h"
#include "GLFW/glfw3.h"
#include "Multimedia/Animation/AnimationSystemDefault.h"
#include "Multimedia/Audio/AudioSystemDefault.h"
#include "Multimedia/Display/DisplaySystemDefault.h"
#include "Multimedia/Rendering/RenderingSystemDefault.h"
#include "Resource/Model.h"
#include "Resource/Shader.h"
#include "filesystem"

int main()
{
    auto &engine = Corona::Engine::inst();
    engine.register_system<Corona::AnimationSystemDefault>();
    engine.register_system<Corona::AudioSystemDefault>();
    engine.register_system<Corona::DisplaySystemDefault>();
    engine.register_system<Corona::RenderingSystemDefault>();
    engine.register_resource_manager<Corona::Model>();
    engine.register_resource_manager<Corona::Shader>();
    engine.get_resource_manager<Corona::Model>().register_loader<Corona::ModelLoader>();
    engine.get_resource_manager<Corona::Shader>().register_loader<Corona::ShaderLoader>();
    engine.init();

    const std::string model_path = (std::filesystem::current_path() / "assets/model/armadillo.obj").string();

    auto model = engine.load<Corona::Model>(model_path);

    Corona::SafeDataCache<Corona::Texture> textureCache;
    Corona::SafeDataCache<Corona::Shader> shaderCache;
    Corona::SafeDataCache<Corona::Model> modelCache;

    auto &renderSystem = engine.get_system<Corona::RenderingSystemDefault>();

    if (glfwInit() >= 0)
    {
        std::vector<GLFWwindow *> windows(4);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        for (size_t i = 0; i < windows.size(); i++)
        {
            windows[i] = glfwCreateWindow(800, 800, "Cabbage Engine", nullptr, nullptr);
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

    // Corona::ResourceManager<Corona::Model>::register_loader<Corona::ModelLoader>();
    //
    // std::thread workthread([&]() {
    //     std::shared_ptr<Corona::Model> model = Corona::ResourceManager<Corona::Model>::load((std::filesystem::current_path()/"assets/model/armadillo.obj").string());
    //     LOG_INFO("Thread 1 Model loaded");
    //     LOG_INFO("Thread 1 Model meshes count : {}", model->meshes.size());
    // });
    //
    // std::thread workthread2([&]() {
    //     std::shared_ptr<Corona::Model> model = Corona::ResourceManager<Corona::Model>::load((std::filesystem::current_path()/"assets/model/armadillo1.obj").string());
    //     LOG_INFO("Thread 2 Model loaded");
    //     LOG_INFO("Thread 2 Model meshes count : {}", model->meshes.size());
    // });
    //
    // std::thread workthread3([&]() {
    //     std::shared_ptr<Corona::Model> model = Corona::ResourceManager<Corona::Model>::load((std::filesystem::current_path()/"assets/model/armadillo1.obj").string());
    //     LOG_INFO("Thread 3 Model loaded");
    //     LOG_INFO("Thread 3 Model meshes count : {}", model->meshes.size());
    // });
    //
    // workthread.join();
    // workthread2.join();
    // workthread3.join();
    //
    // std::shared_ptr<Corona::Model> model = Corona::ResourceManager<Corona::Model>::load((std::filesystem::current_path()/"assets/model/armadillo.obj").string());
    // LOG_INFO("Model loaded");
    // LOG_INFO("Model meshes count : {}", model->meshes.size());

    return 0;
}