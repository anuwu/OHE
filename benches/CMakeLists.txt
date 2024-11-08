add_executable(bench_ohe "bench_ohe.cpp")
target_link_libraries(bench_ohe EMP-OHE)

add_executable(bench_lut "bench_lut.cpp")
target_link_libraries(bench_lut EMP-OHE)

add_executable(bench_fixed "bench_fixed.cpp")
target_link_libraries(bench_fixed EMP-OHE)

add_executable(bench_ferret_budget "bench_ferret_budget.cpp")
target_link_libraries(bench_ferret_budget EMP-OHE)

add_executable(bench_ferret_budget_time "bench_ferret_budget_time.cpp")
target_link_libraries(bench_ferret_budget_time EMP-OHE)