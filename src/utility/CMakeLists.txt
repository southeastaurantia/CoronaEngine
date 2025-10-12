# Corona Utility Libraries - 统一管理和构建
# 提供类似Examples的快捷添加机制

include(CMakeParseArguments)

define_property(GLOBAL PROPERTY _CORONA_UTILITY_TARGETS
    BRIEF_DOCS "Corona utility targets"
    FULL_DOCS "List of Corona utility targets created via corona_add_utility")
define_property(GLOBAL PROPERTY _CORONA_UTILITY_REGISTRY
    BRIEF_DOCS "Corona utility registry"
    FULL_DOCS "Pairs of <module>|<target> captured when corona_add_utility is invoked")
set_property(GLOBAL PROPERTY _CORONA_UTILITY_TARGETS "")
set_property(GLOBAL PROPERTY _CORONA_UTILITY_REGISTRY "")

# 统一的Utility库创建入口
function(corona_add_utility)
    set(options INTERFACE HEADER_ONLY)
    set(oneValueArgs NAME TYPE DESCRIPTION)
    set(multiValueArgs SOURCES PUBLIC_HEADERS PRIVATE_HEADERS DEPENDENCIES)
    cmake_parse_arguments(CORONA_UTIL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT CORONA_UTIL_NAME)
        message(FATAL_ERROR "corona_add_utility requires NAME")
    endif()

    if(NOT CORONA_UTIL_INTERFACE AND NOT CORONA_UTIL_HEADER_ONLY AND NOT CORONA_UTIL_SOURCES)
        message(FATAL_ERROR "corona_add_utility(${CORONA_UTIL_NAME}) requires SOURCES unless INTERFACE/HEADER_ONLY is set")
    endif()

    if(NOT CORONA_UTIL_TYPE)
        set(CORONA_UTIL_TYPE "STATIC")
    endif()

    if(CORONA_UTIL_INTERFACE OR CORONA_UTIL_HEADER_ONLY)
        add_library(${CORONA_UTIL_NAME} INTERFACE)
        set(_corona_lib_scope "INTERFACE")
    else()
        add_library(${CORONA_UTIL_NAME} ${CORONA_UTIL_TYPE} ${CORONA_UTIL_SOURCES})
        set(_corona_lib_scope "PUBLIC")
    endif()

    if(NOT CORONA_UTIL_INTERFACE AND NOT CORONA_UTIL_HEADER_ONLY)
        target_compile_features(${CORONA_UTIL_NAME} PRIVATE cxx_std_20)
    endif()

    target_include_directories(${CORONA_UTIL_NAME}
        ${_corona_lib_scope}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../common/include>
        $<INSTALL_INTERFACE:include>
    )

    if(CORONA_UTIL_DEPENDENCIES)
        target_link_libraries(${CORONA_UTIL_NAME} ${_corona_lib_scope} ${CORONA_UTIL_DEPENDENCIES})
    endif()

    if(COMMAND corona_apply_compile_config)
        corona_apply_compile_config(${CORONA_UTIL_NAME})
    endif()

    string(REGEX REPLACE "^Corona" "Corona::" _corona_alias_name "${CORONA_UTIL_NAME}")
    add_library(${_corona_alias_name} ALIAS ${CORONA_UTIL_NAME})

    set_property(GLOBAL APPEND PROPERTY _CORONA_UTILITY_TARGETS ${CORONA_UTIL_NAME})

    if(DEFINED CORONA_CURRENT_UTILITY_MODULE)
        set_property(GLOBAL APPEND PROPERTY _CORONA_UTILITY_REGISTRY
            "${CORONA_CURRENT_UTILITY_MODULE}|${CORONA_UTIL_NAME}")
    endif()

    message(STATUS "[Corona:Utility] ${CORONA_UTIL_NAME} -> ${_corona_alias_name} (${CORONA_UTIL_TYPE})")

    if(CORONA_UTIL_DESCRIPTION)
        message(STATUS "  - ${CORONA_UTIL_DESCRIPTION}")
    endif()
endfunction()

file(GLOB _corona_candidate_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
list(SORT _corona_candidate_dirs)
set(_corona_built_modules)

foreach(_corona_dir IN LISTS _corona_candidate_dirs)
    if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_corona_dir}")
        continue()
    endif()

    if(_corona_dir STREQUAL "include")
        continue()
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_corona_dir}/CMakeLists.txt")
        set(CORONA_CURRENT_UTILITY_MODULE "${_corona_dir}")
        add_subdirectory(${_corona_dir})
        list(APPEND _corona_built_modules "${_corona_dir}")
    else()
        message(WARNING "[Corona:Utility] ${_corona_dir} missing CMakeLists.txt; skip")
    endif()
endforeach()

unset(CORONA_CURRENT_UTILITY_MODULE)

add_library(CoronaUtilityAll INTERFACE)
get_property(_corona_utility_targets GLOBAL PROPERTY _CORONA_UTILITY_TARGETS)

if(_corona_utility_targets)
    list(FILTER _corona_utility_targets EXCLUDE REGEX "^$")
    list(REMOVE_DUPLICATES _corona_utility_targets)

    foreach(_corona_target IN LISTS _corona_utility_targets)
        target_link_libraries(CoronaUtilityAll INTERFACE ${_corona_target})
    endforeach()
endif()

add_library(Corona::Utility::All ALIAS CoronaUtilityAll)

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Corona Utility Build Summary")
message(STATUS "========================================")

if(_corona_built_modules)
    foreach(_corona_module IN LISTS _corona_built_modules)
        message(STATUS "✓ ${_corona_module}")
    endforeach()
else()
    message(STATUS "(no utility modules discovered)")
endif()

get_property(_corona_registry GLOBAL PROPERTY _CORONA_UTILITY_REGISTRY)

if(_corona_registry)
    list(FILTER _corona_registry EXCLUDE REGEX "^$")
    list(REMOVE_DUPLICATES _corona_registry)
    set(_corona_pairs)

    foreach(_corona_entry IN LISTS _corona_registry)
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

message(STATUS "========================================")
message(STATUS "")
