# ==============================================================================
# corona_runtime_deps.cmake
#
# 功能:
#   运行时依赖 (动态库/调试符号) 收集与复制模块。
#
# 概述:
#   1. 配置阶段: `corona_configure_runtime_deps()` 收集 Python 相关的 DLL/PDB 文件，
#      并写入目标属性 `INTERFACE_CORONA_RUNTIME_DEPS` (设置在 `CoronaEngine` 上)。
#   2. 构建后: `corona_install_runtime_deps(<target>)` 通过自定义命令把上述文件
#      复制到指定目标生成目录，以便示例或可执行程序直接运行。
#
# 设计要点:
#   - 收集逻辑与复制逻辑解耦：收集只跑一次，复制可被多个可执行目标重用。
#   - 使用目标属性避免全局变量污染，便于后续扩展。
#   - 保持幂等：重复调用 configure 函数会覆盖为最新收集结果。
#
# 使用示例:
#   corona_configure_runtime_deps(CoronaEngine)  # 在定义 CoronaEngine 后调用一次
#   corona_install_runtime_deps(MyExampleExe)    # 对每个需要独立运行的可执行目标调用
# ==============================================================================

include_guard(GLOBAL)

# ------------------------------------------------------------------------------
# 函数：安装运行时依赖到目标目录
# ------------------------------------------------------------------------------
function(corona_install_runtime_deps target_name)
    # 从核心库读取之前收集的依赖文件列表
    get_target_property(_CORONA_DEPS CoronaEngine INTERFACE_CORONA_RUNTIME_DEPS)

    if(NOT _CORONA_DEPS)
        message(STATUS "[Corona:RuntimeDeps] No INTERFACE_CORONA_RUNTIME_DEPS; skip copy")
        return()
    endif()

    set(_CORONA_DESTINATION_DIR "$<TARGET_FILE_DIR:${target_name}>")

    # 使用 Python 脚本执行智能复制 (若不同才复制)
    set(_CORONA_PY_COPY "${PROJECT_SOURCE_DIR}/misc/pytools/copy_files.py")

    if(EXISTS "${_CORONA_PY_COPY}" AND DEFINED Python3_EXECUTABLE)
        set(_CORONA_DEPS_DIR "${CMAKE_BINARY_DIR}/runtime_deps")
        file(MAKE_DIRECTORY "${_CORONA_DEPS_DIR}")
        string(MD5 _corona_target_hash "${target_name}")
        set(_CORONA_DEPS_LIST "${_CORONA_DEPS_DIR}/${_corona_target_hash}.txt")
        file(WRITE "${_CORONA_DEPS_LIST}" "")

        foreach(_corona_dep_file IN LISTS _CORONA_DEPS)
            file(APPEND "${_CORONA_DEPS_LIST}" "${_corona_dep_file}\n")
        endforeach()

        add_custom_command(
            TARGET      ${target_name}
            POST_BUILD
            COMMAND     "${Python3_EXECUTABLE}" "${_CORONA_PY_COPY}" --dest "${_CORONA_DESTINATION_DIR}" --list "${_CORONA_DEPS_LIST}"
            COMMENT     "[Corona:RuntimeDeps] Copy Corona runtime dependencies to target directory -> ${target_name}"
            VERBATIM
        )
    else()
        if(NOT EXISTS "${_CORONA_PY_COPY}")
            message(STATUS "[Corona:RuntimeDeps] Python copy script not found; fallback to copy_if_different")
        else()
            message(STATUS "[Corona:RuntimeDeps] Python3 not available; fallback to copy_if_different")
        endif()

        add_custom_command(
            TARGET      ${target_name}
            POST_BUILD
            COMMAND     ${CMAKE_COMMAND} -E copy_if_different ${_CORONA_DEPS} "${_CORONA_DESTINATION_DIR}"
            COMMENT     "[Corona:RuntimeDeps] Copy runtime deps (fallback) -> ${target_name}"
            VERBATIM
        )
    endif()
endfunction()

# ------------------------------------------------------------------------------
# 函数：配置阶段收集运行时依赖
# ------------------------------------------------------------------------------
function(corona_configure_runtime_deps target_name)
    if(NOT TARGET ${target_name})
        message(WARNING "[Corona:RuntimeDeps] Target ${target_name} does not exist; cannot configure runtime dependencies.")
        return()
    endif()

    # 收集 Python 运行库 (DLL/PDB)
    set(_CORONA_ALL_DEPS)
    if(DEFINED Python3_RUNTIME_LIBRARY_DIRS)
        file(GLOB _CORONA_PY_DLLS "${Python3_RUNTIME_LIBRARY_DIRS}/*.dll")
        file(GLOB _CORONA_PY_PDBS "${Python3_RUNTIME_LIBRARY_DIRS}/*.pdb")
        if(_CORONA_PY_DLLS)
            list(APPEND _CORONA_ALL_DEPS ${_CORONA_PY_DLLS})
        endif()
        if(_CORONA_PY_PDBS)
            list(APPEND _CORONA_ALL_DEPS ${_CORONA_PY_PDBS})
        endif()
    endif()

    if(NOT _CORONA_ALL_DEPS)
        message(WARNING "[Corona:RuntimeDeps] No runtime files collected (Python).")
        return()
    endif()

    # 去重，写入目标属性供后续复制使用
    list(REMOVE_DUPLICATES _CORONA_ALL_DEPS)
    set_target_properties(${target_name} PROPERTIES INTERFACE_CORONA_RUNTIME_DEPS "${_CORONA_ALL_DEPS}")
    message(STATUS "[Corona:RuntimeDeps] Collected ${target_name} files: ${_CORONA_ALL_DEPS}")
endfunction()
