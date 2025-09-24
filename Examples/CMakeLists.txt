# =====================================================================================
# 为每个示例提供独立的子目录与构建开关
# =====================================================================================

option(BUILD_EXAMPLE_LFQ_STRESS "Build lock-free queues stress example" ON)
option(BUILD_EXAMPLE_QUEUE_SMOKE "Build simple queue smoke example" ON)
option(BUILD_EXAMPLE_LFQ_CORRECTNESS "Build lock-free queues correctness tests" ON)
option(BUILD_EXAMPLE_QUEUE_USAGE "Build lock-free queues usage samples" ON)

if(BUILD_EXAMPLE_LFQ_STRESS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lfq_stress/CMakeLists.txt")
        add_subdirectory(lfq_stress)
    else()
        message(WARNING "[Examples] lfq_stress subdir not found, skip")
    endif()
endif()

if(BUILD_EXAMPLE_QUEUE_SMOKE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/queue_smoke/CMakeLists.txt")
        add_subdirectory(queue_smoke)
    else()
        message(WARNING "[Examples] queue_smoke subdir not found, skip")
    endif()
endif()

if(BUILD_EXAMPLE_LFQ_CORRECTNESS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lfq_correctness/CMakeLists.txt")
        add_subdirectory(lfq_correctness)
    else()
        message(WARNING "[Examples] lfq_correctness subdir not found, skip")
    endif()
endif()

if(BUILD_EXAMPLE_QUEUE_USAGE)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/queue_usage/CMakeLists.txt")
        add_subdirectory(queue_usage)
    else()
        message(WARNING "[Examples] queue_usage subdir not found, skip")
    endif()
endif()

# 选择 VS 启动项目（若对应 target 存在）
if(BUILD_EXAMPLE_LFQ_STRESS)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Corona_lfq_stress)
elseif(BUILD_EXAMPLE_QUEUE_SMOKE)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Corona_queue_smoke)
elseif(BUILD_EXAMPLE_LFQ_CORRECTNESS)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Corona_lfq_correctness)
elseif(BUILD_EXAMPLE_QUEUE_USAGE)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Corona_queue_usage)
endif()
