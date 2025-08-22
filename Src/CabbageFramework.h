#pragma once

#include <array>
#include <string>

struct CabbageFramework
{
    CabbageFramework() = delete;
    ~CabbageFramework() = delete;

    struct Actor;
    struct Scene;

    struct Actor
    {
        Actor(const std::string &path = "");
        ~Actor();

        void move(const std::array<float, 3> &pos);
        void rotate(const std::array<float, 3> &euler);
        void scale(const std::array<float, 3> &size);

        void setWorldMatrix(const std::array<std::array<float, 4>, 4> &worldMartix);
        std::array<std::array<float, 4>, 4> getWorldMatrix() const;

        void setMeshShape(const std::string &path);
        void setSkeletalAnimation(const std::string &path);

        uint64_t detectCollision(const Actor &other);

        uint64_t getID() const;

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
        uint64_t id;
    };

    struct Scene
    {
        explicit Scene(void *surface = nullptr, bool lightField = false);
        ~Scene();

        void setCamera(const std::array<float, 3> &pos, const std::array<float, 3> &forward, const std::array<float, 3> &worldup, const float &fov);
        void setSunDirection(const std::array<float, 3> &direction);
        void setDisplaySurface(void *surface);

        Actor *detectActorByRay(const std::array<float, 3> &origin, const std::array<float, 3> &dir);

        void addActor(const uint64_t &actor);
        void removeActor(const uint64_t &actor);

        uint64_t getID() const;

      private:
        uint64_t id;
    };
};