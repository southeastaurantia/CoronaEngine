
# ==============================================================================
# Animation System
# ==============================================================================


corona_add_system(animation
    SOURCES kinematics_system.cpp
    DEPENDENCIES
        CoronaResource::Resource
        CabbageHardware
)