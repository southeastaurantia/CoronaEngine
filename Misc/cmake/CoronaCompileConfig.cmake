# CoronaCompileConfig.cmake
# Central place for compile definitions and compiler related settings

add_compile_definitions(
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    NOMINMAX
    ENTT_ID_TYPE=uint64_t
    ENTT_USE_ATOMIC
    FMT_HEADER_ONLY=1
    CORONA_PYTHON_EXE="${Python3_EXECUTABLE}"
    $<$<CONFIG:Debug>:CORONA_ENGINE_DEBUG>
    $<$<CONFIG:RelWithDebInfo>:CORONA_ENGINE_DEBUG>
    $<$<CONFIG:Release>:CORONA_ENGINE_RELEASE>
    $<$<CONFIG:MinSizeRel>:CORONA_ENGINE_RELEASE>
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
