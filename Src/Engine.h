//
// Created by 47226 on 2025/9/8.
//

#ifndef CORONAENGINE_ENGINE_H
#define CORONAENGINE_ENGINE_H
#include <Core/Components.h>
#include <Core/Logger.h>
#include <oneapi/tbb.h>

namespace Corona
{
    struct DataCache
    {
        using id_type = uint64_t;

        static id_type get_next_id();

        tbb::concurrent_hash_map<id_type, std::mutex> animations_mutex;
        tbb::concurrent_hash_map<id_type, std::shared_ptr<Corona::Components::Animations>> animations;

        tbb::concurrent_hash_map<id_type, std::mutex> actor_pose_mutex;
        tbb::concurrent_hash_map<id_type, std::shared_ptr<Corona::Components::ActorPose>> actor_pose;

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

      private:
        Engine();
        ~Engine();
        Engine(const Engine &other) = delete;
        Engine &operator=(const Engine &other) = delete;

        DataCache data;
    };

} // namespace Corona

#endif // CORONAENGINE_ENGINE_H
