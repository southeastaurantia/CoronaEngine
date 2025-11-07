# ==============================================================================
# Optics System
# ==============================================================================

corona_add_system(optics
    SOURCES optics_system.cpp
    DEPENDENCIES 
        CabbageHardware
        CoronaResource::Resource
        $<$<BOOL:${CORONA_BUILD_VISION}>:vision-all_libs>
)
