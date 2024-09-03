cmake_minimum_required (VERSION 3.5)
project(ohe)

set(CMAKE_CXX_FLAGS "-Wall -Wno-sometimes-uninitialized -Wno-unused-const-variable -std=c++11")

find_package(emp-tool)
find_path(EMP-OT_INCLUDE_DIR emp-ot/emp-ot.h)
include_directories(${EMP-TOOL_INCLUDE_DIR})

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${EMP-TOOL_INCLUDE_DIR})

# message("This is ${EMP-TOOL_LIBRARIES}")
# message("This is ${EMP-OT_INCLUDE_DIR}")

add_executable(test "test.cpp")
target_link_libraries(test ${EMP-TOOL_LIBRARIES})

add_executable(multi "multi.cpp")
target_link_libraries(multi ${EMP-TOOL_LIBRARIES})

add_executable(ppohe "pparty_ohe.cpp")
target_link_libraries(ppohe ${EMP-TOOL_LIBRARIES})

add_executable(david "david_ot.cpp")
target_link_libraries(david ${EMP-TOOL_LIBRARIES})

add_executable(vrot "vector_rot.cpp")
target_link_libraries(vrot ${EMP-TOOL_LIBRARIES})

add_executable(throt "check_throt.cpp")
target_link_libraries(throt ${EMP-TOOL_LIBRARIES})

add_executable(tsb "test_send_block.cpp")
target_link_libraries(tsb ${EMP-TOOL_LIBRARIES})

add_executable(ut "util_test.cpp")
target_link_libraries(ut ${EMP-TOOL_LIBRARIES})
