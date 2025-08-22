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