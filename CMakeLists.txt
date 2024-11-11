# Default Project stuff
cmake_minimum_required (VERSION 3.5)
project(ohe)
set(CMAKE_CXX_FLAGS "-std=c++11 -march=native -Wall -Wformat -Wno-sometimes-uninitialized -Wno-unused-const-variable")
include_directories("${CMAKE_CURRENT_LIST_DIR}/src")

message("${CMAKE_CURRENT_LIST_DIR}")

# Find openSSL
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# Find EMP-TOOL
find_package(emp-tool)
find_path(EMP-OT_INCLUDE_DIR emp-ot/emp-ot.h)
include_directories(${EMP-TOOL_INCLUDE_DIR})

# Set directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(verify)
add_subdirectory(benches)
