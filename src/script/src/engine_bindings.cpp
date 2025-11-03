#include <corona/python/engine_scripts.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/string.h>

#include <cstdint>

namespace EngineScripts {

static inline ktm::fvec3 vec3_from(const std::array<float, 3>& a) {
    ktm::fvec3 v{};
    v[0] = a[0];
    v[1] = a[1];
    v[2] = a[2];
    return v;
}

void BindAll(nanobind::module_& m) {
    nanobind::class_<CoronaEngineAPI::Actor>(m, "Actor")
        .def(nanobind::init<const std::string&>(), nanobind::arg("path") = std::string())
        .def("move", [](const CoronaEngineAPI::Actor& self, const std::array<float, 3>& v) {
            self.move(vec3_from(v));
        })
        .def("rotate", [](const CoronaEngineAPI::Actor& self, const std::array<float, 3>& v) {
            self.rotate(vec3_from(v));
        })
        .def("scale", [](const CoronaEngineAPI::Actor& self, const std::array<float, 3>& v) {
            self.scale(vec3_from(v));
        });

    nanobind::class_<CoronaEngineAPI::Scene>(m, "Scene")
        .def(nanobind::init<void*, bool>(), nanobind::arg("surface") = nullptr, nanobind::arg("lightField") = false)
        .def("__init__", [](CoronaEngineAPI::Scene* self, uintptr_t surface, bool lightField) { new (self) CoronaEngineAPI::Scene(reinterpret_cast<void*>(surface), lightField); }, nanobind::arg("surface") = static_cast<uintptr_t>(0), nanobind::arg("lightField") = false)
        .def("setCamera", [](const CoronaEngineAPI::Scene& self, const std::array<float, 3>& position, const std::array<float, 3>& forward, const std::array<float, 3>& worldUp, float fov) { self.setCamera(vec3_from(position), vec3_from(forward), vec3_from(worldUp), fov); })
        .def("setSunDirection", [](const CoronaEngineAPI::Scene& self, const std::array<float, 3>& dir) { self.setSunDirection(vec3_from(dir)); })
        .def("setDisplaySurface", [](CoronaEngineAPI::Scene& self, void* surface) { self.setDisplaySurface(surface); })
        .def("setDisplaySurface", [](CoronaEngineAPI::Scene& self, uintptr_t surface) { self.setDisplaySurface(reinterpret_cast<void*>(surface)); });
}

}  // namespace EngineScripts
