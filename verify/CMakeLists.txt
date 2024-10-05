add_executable(verify_ohe "verify_ohe.cpp")
target_link_libraries(verify_ohe EMP-OHE)

add_executable(verify_lut "verify_lut.cpp")
target_link_libraries(verify_lut EMP-OHE)