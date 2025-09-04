//
// Created by 47226 on 2025/9/4.
//

#include "BaseMultimediaSystem.hpp"
#include "Utils/CabbageLogger.hpp"

#include <chrono>

namespace CabbageFW
{
    BaseMultimediaSystem::BaseMultimediaSystem(const FPS fps)
        : max_fps(fps), engine_is_running(false)
    {
        if (max_fps.load() == 0)
        {
            max_fps.store(60);
        }
    }

    void BaseMultimediaSystem::set_fps(const FPS fps)
    {
        if (fps == 0)
        {
            return;
        }
        max_fps.store(fps);
        LOG_DEBUG(std::format("System '{}' set fps to '{}'", this->name(), max_fps.load()));
    }

    BaseMultimediaSystem::FPS BaseMultimediaSystem::get_fps() const
    {
        return max_fps.load();
    }

    void BaseMultimediaSystem::start()
    {
        this->_start();

        engine_is_running.store(true);
        worker = std::make_unique<std::thread>([this] {
            const int MinFrameSpendTimeSec = 1 / max_fps.load();
            while (engine_is_running.load())
            {
                auto const &now = std::chrono::high_resolution_clock::now();

                this->_tick();

                auto const &end = std::chrono::high_resolution_clock::now();
                if (auto const &duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count() * 1000;
                    duration < MinFrameSpendTimeSec)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(MinFrameSpendTimeSec - duration));
                }
            }
        });
        LOG_DEBUG(std::format("System '{}' started", this->name()));
    }

    void BaseMultimediaSystem::stop()
    {
        this->_stop();

        engine_is_running.store(false);
        if (worker && worker->joinable())
            worker->join();
        LOG_DEBUG(std::format("System '{}' stopped", this->name()));
    }
} // namespace CabbageFW