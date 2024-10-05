add_executable(test_utils "test_utils.cpp")
target_link_libraries(test_utils EMP-OHE)

add_executable(test_coord "test_coord.cpp")
target_link_libraries(test_coord EMP-OHE)

add_executable(test_lut "test_lut.cpp")
target_link_libraries(test_lut EMP-OHE)

add_executable(test_david "test_david.cpp")
target_link_libraries(test_david ${EMP-TOOL_LIBRARIES})

add_executable(test_throttle "test_throttle.cpp")
target_link_libraries(test_throttle ${EMP-TOOL_LIBRARIES})