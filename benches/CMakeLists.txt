add_executable(bench_ohe "bench_ohe.cpp")
target_link_libraries(bench_ohe EMP-OHE)

add_executable(bench_lut "bench_lut.cpp")
target_link_libraries(bench_lut EMP-OHE)