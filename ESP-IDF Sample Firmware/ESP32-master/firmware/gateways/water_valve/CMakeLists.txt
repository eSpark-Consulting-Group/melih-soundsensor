set(SOURCES
    src/WaterValve.cpp
    src/WaterValveGateway.cpp
    src/WaterValveManager.cpp
    src/WaterValve_hard.cpp
    )

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "include"
                       REQUIRES common
                       )
