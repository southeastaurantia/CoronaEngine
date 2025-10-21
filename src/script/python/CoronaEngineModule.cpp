#include <corona/script/EngineScripts.h>
#include <nanobind/nanobind.h>

NB_MODULE(CoronaEngine, m) {
    m.doc() = "CoronaEngine embedded Python module (nanobind)";
    EngineScripts::BindAll(m);
}
