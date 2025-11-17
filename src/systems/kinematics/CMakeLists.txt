
# ==============================================================================
# Animation System
# ==============================================================================


corona_add_system(animation
    SOURCES kinematics_system.cpp
    DEPENDENCIES
        corona::resource::manager
        CabbageHardware
)