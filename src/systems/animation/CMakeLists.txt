
# ==============================================================================
# Animation System
# ==============================================================================


corona_add_system(animation
    SOURCES animation_system.cpp
    DEPENDENCIES
        CoronaResource::Resource
        CabbageHardware
)