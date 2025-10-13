#pragma once

#include <ktm/ktm.h>

struct CoronaEngineAPI {
    CoronaEngineAPI() = delete;
    ~CoronaEngineAPI() = delete;

    struct Actor;
    struct Scene;

    struct Actor {
       public:
        Actor(const std::string& path = "");
        ~Actor();

        void move(ktm::fvec3 pos) const;
        void rotate(ktm::fvec3 euler) const;
        void scale(ktm::fvec3 size) const;

       private:
        uint64_t actorID;
        uint64_t animationID;
    };

    struct Scene {
       public:
        Scene(void* surface = nullptr, bool lightField = false);
        ~Scene();

        void setCamera(const ktm::fvec3& position, const ktm::fvec3& forward, const ktm::fvec3& worldUp, float fov) const;
        void setSunDirection(ktm::fvec3 direction) const;
        void setDisplaySurface(void* surface);

       private:
        uint64_t sceneID;
    };
};
