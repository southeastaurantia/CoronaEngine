//
// Created by 25473 on 25-9-19.
//

#ifndef CORONAENGINEAPI_H
#define CORONAENGINEAPI_H

#include <set>
#include <ktm/ktm.h>
#include "Engine/Engine.h"
#include "Engine/Systems/AnimationSystem.h"
#include "Engine/Systems/RenderingSystem.h"
#include "Resource/AnimationState.h"
#include "Resource/Scene.h"
#include "Resource/Model.h"

namespace CoronaEngine {

    struct CoronaEngineAPI
    {
        CoronaEngineAPI() = delete;
        ~CoronaEngineAPI() = delete;

        struct Actor;
        struct Scene;

        struct Actor
        {
          public:
            Actor(std::string path = "");
            ~Actor();

            struct Pose
            {
                ktm::fvec3 transform = ktm::fvec3(0.0f, 0.0f, 0.0f);
                ktm::fvec3 rotate = ktm::fvec3(0.0f, 0.0f, 0.0f);
                ktm::fvec3 scale = ktm::fvec3(1.0f, 1.0f, 1.0f);
            };

            void move(ktm::fvec3 pos);
            void rotate(ktm::fvec3 euler);
            void scale(ktm::fvec3 size);

            void setPose(const Pose &pose);
            Pose getPose() const;

            void setMeshShape(std::string path);
            void setAnimation(std::string path);
            void updateAnimation(float deltaTime);

            void setActorMatrix(ktm::fmat4x4);
            ktm::fmat4x4 getActorMatrix() const;

            void detectCollision();

            struct OpticsParams
            {
                bool enable;
            };
            void setOpticsParams(const OpticsParams &params);

            struct AcousticsParams
            {
                bool enable;
            };
            void setAcousticsParams(const AcousticsParams &params);

            struct MechanicsParams
            {
                bool enable;
            };
            void setMechanicsParams(const MechanicsParams &params);

          private:
            Pose pose;
        };

        struct Scene
        {
          public:
            Scene(void *surface = nullptr, bool lightField = false);
            ~Scene();

            void setCamera(const ktm::fvec3 &position, const ktm::fvec3 &forward, const ktm::fvec3 &worldUp, float fov);

            void setSunDirection(ktm::fvec3 direction);
            ktm::fvec3 getSunDirection() const;

            void setDisplaySurface(void *surface);
            void *getDisplaySurface() const;

            Actor *detectActorByRay(ktm::fvec3 origin, ktm::fvec3 dir);
            Actor *detectActorByScreen(ktm::uvec2 pixel);

            void update(float deltaTime);

            std::set<Actor *> getActors() const;

          private:
            uint64_t sceneID;
        };

    private:
        auto &sceneCache = Corona::Engine::Instance().Cache<Corona::Scene>();
        auto &modelCache = Corona::Engine::Instance().Cache<Corona::Model>();
        auto &animStateCache = Corona::Engine::Instance().Cache<Corona::AnimationState>();
        auto &renderingSystem = Corona::Engine::Instance().GetSystem<Corona::RenderingSystem>();
        auto &animationSystem = Corona::Engine::Instance().GetSystem<Corona::AnimationSystem>();
        auto &render_queue = Corona::Engine::Instance().GetQueue(renderingSystem.name());
        auto &anim_queue = Corona::Engine::Instance().GetQueue(animationSystem.name());
    };

} // CoronaEngine

#endif //CORONAENGINEAPI_H
