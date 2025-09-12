//
// Created by 47226 on 2025/9/8.
//

#ifndef CORONAENGINE_ENGINE_H
#define CORONAENGINE_ENGINE_H
#include "Multimedia/BaseMultimediaSystem.hpp"
#include "Thread/SafeCommandQueue.hpp"

#include <Core/Components.h>
#include <Core/Logger.h>
#include <oneapi/tbb.h>
#include <typeindex>

namespace Corona
{
    struct DataCache
    {
        using id_type = uint64_t;

        static id_type get_next_id();

        tbb::concurrent_hash_map<id_type, std::mutex> actor_pose_mutex;
        tbb::concurrent_hash_map<id_type, std::shared_ptr<Corona::Components::ActorPose>> actor_pose;

        tbb::concurrent_hash_map<id_type, std::mutex> animations_mutex;
        tbb::concurrent_hash_map<id_type, std::shared_ptr<Corona::Components::Animations>> animations;

      private:
        static std::atomic<id_type> id_counter;
    };

    class Engine final
    {
      public:
        static Engine &inst();

        void init();
        DataCache &data_cache();
        const DataCache &data_cache() const;

        Logger &logger() const;

        template<typename T>
            requires std::is_base_of_v<BaseMultimediaSystem, T>
        void register_system()
        {
            if (systems.contains(std::type_index(typeid(T))))
            {
                return;
            }
            systems.emplace(std::type_index(typeid(T)), std::make_shared<T>());
            LOG_DEBUG("Registered system {}", std::type_index(typeid(T)).name());
        }

        template<typename T>
            requires std::is_base_of_v<BaseMultimediaSystem, T>
        T &get_system() const
        {
            return *std::static_pointer_cast<T>(systems.at(std::type_index(typeid(T))));
        }

        SafeCommandQueue* get_cmd_queue(const std::string& system_name) const;
        void add_cmd_queue(const std::string& system_name, std::unique_ptr<SafeCommandQueue> cmd_queue);

      private:
        Engine();
        ~Engine();
        Engine(const Engine &other) = delete;
        Engine &operator=(const Engine &other) = delete;

        tbb::concurrent_unordered_map<std::string, std::unique_ptr<SafeCommandQueue>> system_cmd_queues;
        std::shared_ptr<Logger> engineLogger;
        std::unordered_map<std::string, std::shared_ptr<BaseMultimediaSystem>> systems;

        DataCache data;
    };

} // namespace Corona

#endif // CORONAENGINE_ENGINE_H
