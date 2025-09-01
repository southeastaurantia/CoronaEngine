//
// Created by 47226 on 2025/8/22.
//

#include "BackBridge.h"

entt::dispatcher &BackBridge::anim_dispatcher()
{
    static entt::dispatcher inst;
    return inst;
}

entt::dispatcher &BackBridge::audio_dispatcher()
{
    static entt::dispatcher inst;
    return inst;
}

entt::dispatcher &BackBridge::render_dispatcher()
{
    static entt::dispatcher inst;
    return inst;
}

BackBridge::SceneToActorsMap &BackBridge::scene_to_actors()
{
    static SceneToActorsMap inst;
    return inst;
}

BackBridge::ActorToScenesMap &BackBridge::actor_to_scenes()
{
    static ActorToScenesMap inst;
    return inst;
}
