set(_target Corona_queue_usage)
add_executable(${_target} queue_usage.cpp)
set_target_properties(${_target} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>)
target_link_libraries(${_target} PRIVATE glfw CoronaEngine)

if(COMMAND helicon_install_runtime_deps)
    helicon_install_runtime_deps(${_target})
else()
    message(STATUS "[queue_usage] helicon_install_runtime_deps not available; skip")
endif()
corona_install_runtime_deps(${_target})
if(BUILD_CORONA_EDITOR)
    corona_install_corona_editor(${_target} CoronaEngine)
endif()

add_custom_command(TARGET ${_target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "[queue_usage] Copy assets -> $<TARGET_FILE_DIR:${_target}>/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${_target}>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/../assets" $<TARGET_FILE_DIR:${_target}>/assets
    VERBATIM)
