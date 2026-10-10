cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(CPUSL)

add_subdirectory(CPUSL_LIB)
add_subdirectory(Test)

target_link_libraries(Test PRIVATE CPUSL_LIB)
