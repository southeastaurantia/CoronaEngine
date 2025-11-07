# ==============================================================================
# Optics System
# ==============================================================================

corona_add_system(optics
    SOURCES optics_system.cpp
    DEPENDENCIES 
        CabbageHardware
        CoronaResource::Resource
        vision-all_libs
    INCLUDE_DIRS
        ${Vision_SOURCE_DIR}/src
)
