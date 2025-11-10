#include <corona/script/python/engine_scripts.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/string.h>

#include <array>
#include <cstdint>

#include "Codegen/BuiltinVariate.h"

namespace EngineScripts {

static inline ktm::fvec3 vec3_from(const std::array<float, 3>& a) {
    ktm::fvec3 v;
    v.x = a[0];
    v.y = a[1];
    v.z = a[2];
    return v;
}

void BindAll(nanobind::module_& m) {
    // Actor bindings
    nanobind::class_<CoronaEngineAPI::Actor>(m, "Actor")
        .def(nanobind::init<const std::string&>(), nanobind::arg("path") = std::string())
        .def("move", [](const CoronaEngineAPI::Actor& self, const std::array<float, 3>& v) { self.move(vec3_from(v)); }, nanobind::arg("position"))
        .def("rotate", [](const CoronaEngineAPI::Actor& self, const std::array<float, 3>& v) { self.rotate(vec3_from(v)); }, nanobind::arg("euler"))
        .def("scale", [](const CoronaEngineAPI::Actor& self, const std::array<float, 3>& v) { self.scale(vec3_from(v)); }, nanobind::arg("size"));

    // Camera bindings
    nanobind::class_<CoronaEngineAPI::Camera>(m, "Camera")
        .def(nanobind::init<>())
        .def("__init__", [](CoronaEngineAPI::Camera* self, const std::array<float, 3>& position, const std::array<float, 3>& forward, const std::array<float, 3>& world_up, float fov) { new (self) CoronaEngineAPI::Camera(vec3_from(position), vec3_from(forward), vec3_from(world_up), fov); }, nanobind::arg("position"), nanobind::arg("forward"), nanobind::arg("world_up"), nanobind::arg("fov"))
        .def("set_surface", [](const CoronaEngineAPI::Camera& self, std::uintptr_t surface) { self.set_surface(reinterpret_cast<void*>(surface)); }, nanobind::arg("surface"))
        .def("set_position", [](const CoronaEngineAPI::Camera& self, const std::array<float, 3>& v) { self.set_position(vec3_from(v)); }, nanobind::arg("position"))
        .def("get_position", [](const CoronaEngineAPI::Camera& self) -> std::array<float, 3> {
            auto v = self.get_position();
            return {{v.x, v.y, v.z}}; })
        .def("set_forward", [](const CoronaEngineAPI::Camera& self, const std::array<float, 3>& v) { self.set_forward(vec3_from(v)); }, nanobind::arg("forward"))
        .def("get_forward", [](const CoronaEngineAPI::Camera& self) -> std::array<float, 3> {
            auto v = self.get_forward();
            return {{v.x, v.y, v.z}}; })
        .def("set_world_up", [](const CoronaEngineAPI::Camera& self, const std::array<float, 3>& v) { self.set_world_up(vec3_from(v)); }, nanobind::arg("world_up"))
        .def("get_world_up", [](const CoronaEngineAPI::Camera& self) -> std::array<float, 3> {
            auto v = self.get_world_up();
            return {{v.x, v.y, v.z}}; })
        .def("set_fov", &CoronaEngineAPI::Camera::set_fov, nanobind::arg("fov"))
        .def("get_fov", &CoronaEngineAPI::Camera::get_fov)
        .def("move", [](const CoronaEngineAPI::Camera& self, const std::array<float, 3>& v) { self.move(vec3_from(v)); }, nanobind::arg("delta"))
        .def("rotate", [](const CoronaEngineAPI::Camera& self, const std::array<float, 3>& v) { self.rotate(vec3_from(v)); }, nanobind::arg("euler"))
        .def("look_at", [](const CoronaEngineAPI::Camera& self, const std::array<float, 3>& pos, const std::array<float, 3>& forward) { self.look_at(vec3_from(pos), vec3_from(forward)); }, nanobind::arg("position"), nanobind::arg("forward"));

    // Light bindings
    nanobind::class_<CoronaEngineAPI::Light>(m, "Light")
        .def(nanobind::init<>());

    // Scene bindings
    nanobind::class_<CoronaEngineAPI::Scene>(m, "Scene")
        .def(nanobind::init<const bool>(), nanobind::arg("light_field") = false)
        .def("set_sun_direction", [](const CoronaEngineAPI::Scene& self, const std::array<float, 3>& dir) { self.set_sun_direction(vec3_from(dir)); }, nanobind::arg("direction"))
        .def("add_camera", &CoronaEngineAPI::Scene::add_camera, nanobind::arg("camera"))
        .def("add_light", &CoronaEngineAPI::Scene::add_light, nanobind::arg("light"))
        .def("add_actor", &CoronaEngineAPI::Scene::add_actor, nanobind::arg("actor"))
        .def("remove_camera", &CoronaEngineAPI::Scene::remove_camera, nanobind::arg("camera"))
        .def("remove_light", &CoronaEngineAPI::Scene::remove_light, nanobind::arg("light"))
        .def("remove_actor", &CoronaEngineAPI::Scene::remove_actor, nanobind::arg("actor"));
}

}  // namespace EngineScripts
