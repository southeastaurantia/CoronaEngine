#pragma once

#include <Multimedia/Animation/AnimationSystemDefault.hpp>
#include <Multimedia/Audio/AudioSystemDefault.hpp>
#include <Multimedia/Display/DisplaySystemDefault.hpp>
#include <Multimedia/Rendering/RenderingSystemDefault.hpp>
#include <array>
#include <string>

namespace CabbageFW
{
    struct ActorImpl;
    struct SceneImpl;

    struct Actor final
    {
        explicit Actor(const std::string &path = "");
        Actor(const Actor &other);
        Actor(Actor &&other) noexcept;
        ~Actor();

        Actor &operator=(const Actor &other);
        Actor &operator=(Actor &&other) noexcept;

        void move(const std::array<float, 3> &pos) const;
        void rotate(const std::array<float, 3> &euler) const;
        void scale(const std::array<float, 3> &size) const;

        void setWorldMatrix(const std::array<std::array<float, 4>, 4> &worldMartix) const;
        [[nodiscard]] std::array<std::array<float, 4>, 4> getWorldMatrix() const;

        void setMeshShape(const std::string &path) const;
        void setSkeletalAnimation(const std::string &path) const;

        uint64_t detectCollision(const ActorImpl &other);

        [[nodiscard]] uint64_t getID() const;
        ActorImpl *get() const;

      private:
        ActorImpl *impl;
        int *ref_count;
    };

    struct Scene final
    {
        explicit Scene(void *surface = nullptr, bool lightField = false);
        Scene(const Scene &other);
        Scene(Scene &&other) noexcept;
        ~Scene();

        Scene &operator=(const Scene &other);
        Scene &operator=(Scene &&other) noexcept;

        void setCamera(const std::array<float, 3> &pos, const std::array<float, 3> &forward, const std::array<float, 3> &worldup, const float &fov) const;
        void setSunDirection(const std::array<float, 3> &direction) const;
        void setDisplaySurface(void *surface) const;

        [[nodiscard]] uint64_t detectActorByRay(const std::array<float, 3> &origin, const std::array<float, 3> &dir) const;

        void addActor(const Actor &actor) const;
        void removeActor(const Actor &actor) const;

        [[nodiscard]] uint64_t getID() const;
        SceneImpl *get() const;

      private:
        SceneImpl *impl;
        int *ref_count;
    };
}; // namespace CabbageFW
