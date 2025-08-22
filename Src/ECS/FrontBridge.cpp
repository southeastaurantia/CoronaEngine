//
// Created by 47226 on 2025/8/22.
//

#include "FrontBridge.h"

entt::dispatcher &FrontBridge::dispatcher()
{
    static entt::dispatcher inst;
    return inst;
}