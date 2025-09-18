# =====================================================================================
#  示例源码收集 (Example Sources)
#  说明：递归抓取示例目录下全部 C/C++ 源/头；若为空给出警告，提醒是否遗失示例或路径配置问题。
# =====================================================================================
file(GLOB_RECURSE EXAMPLES_SOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)
if(NOT EXAMPLES_SOURCE_FILES)
    message(WARNING "[Examples] No example source files found in ${CMAKE_CURRENT_SOURCE_DIR}.")
endif()
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${EXAMPLES_SOURCE_FILES})

# =====================================================================================
#  可执行目标 (Executable Target)
#  说明：统一生成单一示例程序 CoronaEngineExamples，后续可拆分为多个具体演示（可按主题新增 target）。
# =====================================================================================
add_executable(CoronaEngineExamples ${EXAMPLES_SOURCE_FILES})

# VS 启动项目与调试工作目录：保证在 VS/IDE 中按 F5 直接运行时，工作目录等于二进制输出目录，便于资源相对路径加载。
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT CoronaEngineExamples)
set_target_properties(CoronaEngineExamples PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:CoronaEngineExamples>
)

target_link_libraries(CoronaEngineExamples PRIVATE
    glfw            # 窗口 & 输入
    CoronaEngine    # 引擎主库
)

# =====================================================================================
#  运行时依赖复制 (Runtime Dependencies)
#  说明：分两个层面
#    1) helicon_install_runtime_deps: Helicon 组件自身的运行库复制（外部模块提供）
#    2) corona_install_runtime_deps: 引擎汇总的 TBB / Python 等动态库复制
#  目的：保证直接在构建输出目录执行示例可运行，不需手动拷贝 DLL。
# =====================================================================================
helicon_install_runtime_deps(CoronaEngineExamples)
corona_install_runtime_deps(CoronaEngineExamples)
corona_install_corona_editor(CoronaEngineExamples CoronaEngine)

# =====================================================================================
#  资源目录拷贝 (Asset Copy Step)
#  说明：将 Examples/assets 整目录复制到示例可执行输出目录的 assets/ 下；若目录为空仍会创建。
#        可在此追加着色器编译、纹理压缩等后处理步骤。
# =====================================================================================
set(_EXAMPLES_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
add_custom_command(
    TARGET CoronaEngineExamples POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "[Examples] Copying assets (if any) -> $<TARGET_FILE_DIR:CoronaEngineExamples>/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:CoronaEngineExamples>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${_EXAMPLES_ASSETS_DIR}"
            $<TARGET_FILE_DIR:CoronaEngineExamples>/assets
    COMMENT "[Examples] Copy asset directory"
    VERBATIM
)
