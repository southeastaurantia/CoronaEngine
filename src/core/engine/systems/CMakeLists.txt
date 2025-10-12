include(CMakeParseArguments)

define_property(GLOBAL PROPERTY _CORONA_ENGINE_SYSTEM_TARGETS
    BRIEF_DOCS "Corona engine system targets"
    FULL_DOCS "List of Corona engine system targets created via corona_add_engine_system")
define_property(GLOBAL PROPERTY _CORONA_ENGINE_SYSTEM_REGISTRY
    BRIEF_DOCS "Corona engine system registry"
    FULL_DOCS "Pairs of <module>|<target> captured when corona_add_engine_system is invoked")
set_property(GLOBAL PROPERTY _CORONA_ENGINE_SYSTEM_TARGETS "")
set_property(GLOBAL PROPERTY _CORONA_ENGINE_SYSTEM_REGISTRY "")

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

    if(DEFINED CORONA_CURRENT_SYSTEM_MODULE)
        set_property(GLOBAL APPEND PROPERTY _CORONA_ENGINE_SYSTEM_REGISTRY
            "${CORONA_CURRENT_SYSTEM_MODULE}|${CORONA_SYS_NAME}")
    endif()

    message(STATUS "[Corona:System] ${CORONA_SYS_NAME} configured")
endfunction()

file(GLOB _corona_system_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
list(SORT _corona_system_dirs)
set(_corona_built_systems)

foreach(_corona_system_module IN LISTS _corona_system_dirs)
    if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_corona_system_module}")
        continue()
    endif()

    if(_corona_system_module STREQUAL "CMakeFiles")
        continue()
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_corona_system_module}/CMakeLists.txt")
        set(CORONA_CURRENT_SYSTEM_MODULE "${_corona_system_module}")
        add_subdirectory(${_corona_system_module})
        list(APPEND _corona_built_systems "${_corona_system_module}")
    else()
        message(WARNING "[Corona:System] ${_corona_system_module} module missing CMakeLists.txt")
    endif()
endforeach()

unset(CORONA_CURRENT_SYSTEM_MODULE)

get_property(_corona_system_targets GLOBAL PROPERTY _CORONA_ENGINE_SYSTEM_TARGETS)

if(NOT _corona_system_targets)
    set(_corona_system_targets)
endif()

add_library(CoronaSystemsAll INTERFACE)

if(_corona_system_targets)
    list(FILTER _corona_system_targets EXCLUDE REGEX "^$")
    list(REMOVE_DUPLICATES _corona_system_targets)
endif()

foreach(_corona_target ${_corona_system_targets})
    target_link_libraries(CoronaSystemsAll INTERFACE ${_corona_target})
endforeach()

add_library(Corona::Systems::All ALIAS CoronaSystemsAll)

message(STATUS "")
message(STATUS "================ Corona Systems Summary ================")

if(_corona_built_systems)
    foreach(_corona_module IN LISTS _corona_built_systems)
        message(STATUS "✓ ${_corona_module}")
    endforeach()
else()
    message(STATUS "(no engine systems discovered)")
endif()

get_property(_corona_system_registry GLOBAL PROPERTY _CORONA_ENGINE_SYSTEM_REGISTRY)

if(_corona_system_registry)
    list(FILTER _corona_system_registry EXCLUDE REGEX "^$")
    list(REMOVE_DUPLICATES _corona_system_registry)
    set(_corona_pairs)

    foreach(_corona_entry IN LISTS _corona_system_registry)
        string(REPLACE "|" ";" _corona_split "${_corona_entry}")
        list(LENGTH _corona_split _corona_split_len)

        if(_corona_split_len EQUAL 2)
            list(GET _corona_split 0 _corona_module_name)
            list(GET _corona_split 1 _corona_target_name)
            list(APPEND _corona_pairs "${_corona_module_name}->${_corona_target_name}")
        else()
            list(APPEND _corona_pairs "${_corona_entry}")
        endif()
    endforeach()

    string(JOIN ", " _corona_pairs_joined ${_corona_pairs})
    message(STATUS "Linked targets: ${_corona_pairs_joined}")
else()
    message(STATUS "Linked targets: (none)")
endif()

message(STATUS "========================================================")
message(STATUS "")
