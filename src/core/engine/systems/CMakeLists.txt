include(CMakeParseArguments)

# Registry for created system targets
define_property(GLOBAL PROPERTY _CORONA_ENGINE_SYSTEM_TARGETS
    BRIEF_DOCS "Corona engine system targets"
    FULL_DOCS "List of Corona engine system targets created via corona_add_engine_system")
set_property(GLOBAL PROPERTY _CORONA_ENGINE_SYSTEM_TARGETS "")

function(corona_add_engine_system)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES DEPENDENCIES)
    cmake_parse_arguments(CORONA_SYS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT CORONA_SYS_NAME)
        message(FATAL_ERROR "corona_add_engine_system requires NAME")
    endif()

    if(NOT CORONA_SYS_SOURCES)
        message(FATAL_ERROR "corona_add_engine_system(${CORONA_SYS_NAME}) requires SOURCES")
    endif()

    add_library(${CORONA_SYS_NAME} STATIC ${CORONA_SYS_SOURCES})
    target_compile_features(${CORONA_SYS_NAME} PRIVATE cxx_std_20)

    target_include_directories(${CORONA_SYS_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/include>
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/common/include>
            $<INSTALL_INTERFACE:include>
    )

    if(DEFINED CORONA_ENGINE_SYSTEM_DEPENDENCIES)
        target_link_libraries(${CORONA_SYS_NAME} PUBLIC ${CORONA_ENGINE_SYSTEM_DEPENDENCIES})
    endif()

    if(CORONA_SYS_DEPENDENCIES)
        target_link_libraries(${CORONA_SYS_NAME} PUBLIC ${CORONA_SYS_DEPENDENCIES})
    endif()

    if(TARGET CoronaUtilityAll)
        target_link_libraries(${CORONA_SYS_NAME} PUBLIC CoronaUtilityAll)
    endif()

    if(COMMAND corona_apply_compile_config)
        corona_apply_compile_config(${CORONA_SYS_NAME})
    endif()

    add_library(Corona::System::${CORONA_SYS_NAME} ALIAS ${CORONA_SYS_NAME})

    set_property(GLOBAL APPEND PROPERTY _CORONA_ENGINE_SYSTEM_TARGETS ${CORONA_SYS_NAME})

    message(STATUS "[Corona:System] ${CORONA_SYS_NAME} configured")
endfunction()

set(_CORONA_ENGINE_SYSTEM_MODULES
    animation
    audio
    display
    rendering
)

foreach(_corona_system_module ${_CORONA_ENGINE_SYSTEM_MODULES})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_corona_system_module}/CMakeLists.txt")
        add_subdirectory(${_corona_system_module})
    else()
        message(WARNING "[Corona:System] ${_corona_system_module} module missing CMakeLists.txt")
    endif()
endforeach()

get_property(_corona_system_targets GLOBAL PROPERTY _CORONA_ENGINE_SYSTEM_TARGETS)
if(NOT _corona_system_targets)
    set(_corona_system_targets)
endif()

add_library(CoronaSystemsAll INTERFACE)
foreach(_corona_target ${_corona_system_targets})
    target_link_libraries(CoronaSystemsAll INTERFACE ${_corona_target})
endforeach()

add_library(Corona::Systems::All ALIAS CoronaSystemsAll)
