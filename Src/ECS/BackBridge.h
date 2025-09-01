//
// Created by 47226 on 2025/8/22.
//

#ifndef CABBAGEFRAMEWORK_BACKBRIDGE_H
#define CABBAGEFRAMEWORK_BACKBRIDGE_H

#include <entt/entt.hpp>
#include <oneapi/tbb.h>

class BackBridge
{
  public:
    using SceneToActorsMap = tbb::concurrent_unordered_map<entt::entity, tbb::concurrent_unordered_set<entt::entity>>;
    using ActorToScenesMap = tbb::concurrent_unordered_map<entt::entity, tbb::concurrent_unordered_set<entt::entity>>;  
    
    static entt::dispatcher &anim_dispatcher();
    static entt::dispatcher &audio_dispatcher();
    static entt::dispatcher &render_dispatcher();
    static SceneToActorsMap &scene_to_actors();
    static ActorToScenesMap &actor_to_scenes();

};

#endif // CABBAGEFRAMEWORK_BACKBRIDGE_H
