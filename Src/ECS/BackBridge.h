//
// Created by 47226 on 2025/8/22.
//

#ifndef CABBAGEFRAMEWORK_BACKBRIDGE_H
#define CABBAGEFRAMEWORK_BACKBRIDGE_H

#include <entt/entt.hpp>

class BackBridge
{
  public:
    static entt::dispatcher &anim_dispatcher();
    static entt::dispatcher &audio_dispatcher();
    static entt::dispatcher &render_dispatcher();
};

#endif // CABBAGEFRAMEWORK_BACKBRIDGE_H
