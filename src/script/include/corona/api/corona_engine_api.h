#pragma once

#include <ktm/ktm.h>

#include <entt/entt.hpp>
#include <string>

namespace Corona {
    class Model;
}

struct CoronaEngineAPI {
    CoronaEngineAPI() = delete;
    ~CoronaEngineAPI() = delete;

    struct RenderTag {};
    struct AnimationTag {};
    struct AudioTag {};
    struct DisplayTag {};

    struct Base {
       protected:
        Base() : id_(registry_.create()) {}
        ~Base() = default;

       public:
        [[nodiscard]] entt::entity get_id() const { return id_; }

       protected:
        entt::entity id_{};
    };

    struct Actor : public Base {
       public:
        explicit Actor(const std::string& path = "");
        ~Actor();

        [[nodiscard]] std::uintptr_t get_handle_id() const;

        void move(ktm::fvec3 pos) const;
        void rotate(ktm::fvec3 euler) const;
        void scale(ktm::fvec3 size) const;

       private:
        std::uintptr_t model_handle_{};
        std::uintptr_t device_handle_{};
    };

    struct Light : public Base {
       public:
        Light();
        ~Light();

        [[nodiscard]] std::uintptr_t get_handle_id() const;

       private:
        std::uintptr_t handle_{};
    };

    struct Camera : public Base {
       public:
        Camera();
        Camera(const ktm::fvec3& position, const ktm::fvec3& forward, const ktm::fvec3& world_up, float fov);
        ~Camera();

        [[nodiscard]] std::uintptr_t get_handle_id() const;

        void set_surface(void* surface) const;

       private:
        std::uintptr_t handle_{};
    };

    struct Scene {
       public:
        explicit Scene(bool light_field = false);
        ~Scene();

        // void set_camera(const ktm::fvec3& position, const ktm::fvec3& forward, const ktm::fvec3& world_up, float fov) const;
        // void set_display_surface(void* surface) const;
        void set_sun_direction(ktm::fvec3 direction) const;

        void add_camera(const Camera& camera) const;
        void add_light(const Light& light) const;
        void add_actor(const Actor& actor) const;

        void remove_camera(const Camera& camera) const;
        void remove_light(const Light& light) const;
        void remove_actor(const Actor& actor) const;

       private:
        entt::entity scene_id_{};
        std::uintptr_t scene_handle_{};
    };


   private:
    static entt::registry registry_;
};
