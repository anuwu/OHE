add_library(EMP-OHE utils.cpp ohe.cpp coord.cpp lut.cpp)
target_link_libraries(EMP-OHE INTERFACE ${EMP-TOOL_LIBRARIES})