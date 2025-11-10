# ==============================================================================
# Mechanics System
# ==============================================================================

corona_add_system(mechanics
    SOURCES mechanics_system.cpp
    DEPENDENCIES
        CabbageHardware
        CoronaResource::Resource
)
