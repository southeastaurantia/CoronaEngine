#pragma once

#include <ktm/ktm.h>

#include <entt/entt.hpp>

struct CoronaEngineAPI {
    CoronaEngineAPI() = delete;
    ~CoronaEngineAPI() = delete;

    struct RenderTag {};
    struct AnimationTag {};
    struct AudioTag {};
    struct DisplayTag {};

    struct Actor {
       public:
        Actor(const std::string& path = "");
        ~Actor();

        void move(ktm::fvec3 pos) const;
        void rotate(ktm::fvec3 euler) const;
        void scale(ktm::fvec3 size) const;

       private:
        entt::entity actorID;
    };

    struct Scene {
       public:
        Scene(void* surface = nullptr, bool lightField = false);
        ~Scene();

        void setCamera(const ktm::fvec3& position, const ktm::fvec3& forward, const ktm::fvec3& worldUp, float fov) const;
        void setSunDirection(ktm::fvec3 direction) const;
        void setDisplaySurface(void* surface);

       private:
        entt::entity sceneID;
    };

private:
    static entt::registry registry_;
};
