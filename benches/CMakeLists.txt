add_executable(bench_ohe_old "bench_ohe_old.cpp")
target_link_libraries(bench_ohe_old EMP-OHE)

add_executable(bench_ohe "bench_ohe.cpp")
target_link_libraries(bench_ohe EMP-OHE)