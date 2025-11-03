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
        entt::entity actor_id_;
    };

    struct Scene {
       public:
        Scene(void* surface = nullptr, bool light_field = false);
        ~Scene();

        void set_camera(const ktm::fvec3& position, const ktm::fvec3& forward, const ktm::fvec3& world_up, float fov) const;
        void set_sun_direction(ktm::fvec3 direction) const;
        void set_display_surface(void* surface);

       private:
        entt::entity scene_id_;
    };

   private:
    static entt::registry registry_;
};
