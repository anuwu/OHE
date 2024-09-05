# Default Project stuff
cmake_minimum_required (VERSION 3.5)
project(ohe)
set(CMAKE_CXX_FLAGS "-Wall -Wno-sometimes-uninitialized -Wno-unused-const-variable -std=c++11")
include_directories(${CMAKE_CURRENT_LIST_DIR})

# Find openSSL
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# Find EMP-TOOL
find_package(emp-tool)
find_path(EMP-OT_INCLUDE_DIR emp-ot/emp-ot.h)
include_directories(${EMP-TOOL_INCLUDE_DIR})

# Build EMP-OHE
add_library(EMP-OHE utils.cpp ohe.cpp ohe_old.cpp lut.cpp)
target_link_libraries(EMP-OHE INTERFACE ${EMP-TOOL_LIBRARIES})

# Add subdirectories
add_subdirectory(benches)
add_subdirectory(tests)