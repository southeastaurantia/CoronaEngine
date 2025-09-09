#include "Core/Engine.h"
#include "Core/Thread/SafeDataCache.h"
#include "Multimedia/Animation/AnimationSystemDefault.hpp"
#include "Multimedia/Audio/AudioSystemDefault.hpp"
#include "Multimedia/Display/DisplaySystemDefault.hpp"
#include "Multimedia/Rendering/RenderingSystemDefault.hpp"

#include <Core/IO/ResMgr.h>
#include <Core/Logger.h>
#include <chrono>
#include <iostream>

int main()
{
    Corona::Engine::inst().init();

    return 0;
}