//
// Created by 25473 on 25-9-19.
//

#ifndef CORONAENGINEAPI_H
#define CORONAENGINEAPI_H

#include <ktm/ktm.h>
#include "core/engine/Engine.h"
#include "core/engine/systems/AnimationSystem.h"
#include "core/engine/systems/RenderingSystem.h"
#include "resource/AnimationState.h"
#include "resource/Scene.h"
#include "resource/Model.h"

struct CoronaEngineAPI
{
    CoronaEngineAPI() = delete;
    ~CoronaEngineAPI() = delete;

    struct Actor;
    struct Scene;

    struct Actor
    {
      public:
        Actor(const std::string &path = "");
        ~Actor();

        void move(ktm::fvec3 pos) const;
        void rotate(ktm::fvec3 euler) const;
        void scale(ktm::fvec3 size) const;

      private:
        uint64_t actorID;
        uint64_t animationID;
    };

    struct Scene
    {
      public:
        Scene(void *surface = nullptr, bool lightField = false);
        ~Scene();

        void setCamera(const ktm::fvec3 &position, const ktm::fvec3 &forward, const ktm::fvec3 &worldUp, float fov) const;
        void setSunDirection(ktm::fvec3 direction) const;
        void setDisplaySurface(void *surface);

      private:
        uint64_t sceneID;
    };
};

#endif //CORONAENGINEAPI_H
