#include "Core/Engine.h"
#include "Core/IO/ResMgr.h"
#include "Resource/Model.h"
#include "filesystem"

int main()
{
    Corona::Engine::inst().init();

    Corona::ResMgr<Corona::Model>::register_loader<Corona::ModelLoader>();

    std::thread workthread([&]() {
        std::shared_ptr<Corona::Model> model = Corona::ResMgr<Corona::Model>::load((std::filesystem::current_path()/"assets/model/armadillo.obj").string());
        LOG_INFO("Thread 1 Model loaded");
        LOG_INFO("Thread 1 Model meshes count : {}", model->meshes.size());
    });

    std::thread workthread2([&]() {
        std::shared_ptr<Corona::Model> model = Corona::ResMgr<Corona::Model>::load((std::filesystem::current_path()/"assets/model/armadillo1.obj").string());
        LOG_INFO("Thread 2 Model loaded");
        LOG_INFO("Thread 2 Model meshes count : {}", model->meshes.size());
    });

    std::thread workthread3([&]() {
        std::shared_ptr<Corona::Model> model = Corona::ResMgr<Corona::Model>::load((std::filesystem::current_path()/"assets/model/armadillo1.obj").string());
        LOG_INFO("Thread 3 Model loaded");
        LOG_INFO("Thread 3 Model meshes count : {}", model->meshes.size());
    });

    workthread.join();
    workthread2.join();
    workthread3.join();

    std::shared_ptr<Corona::Model> model = Corona::ResMgr<Corona::Model>::load((std::filesystem::current_path()/"assets/model/armadillo.obj").string());
    LOG_INFO("Model loaded");
    LOG_INFO("Model meshes count : {}", model->meshes.size());

    std::cin.get();

    return 0;
}