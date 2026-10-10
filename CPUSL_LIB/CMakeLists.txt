cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(CPUSL_LIB)

file(GLOB LIB_SOURCES 
	src/h/*.h
	src/cpp/*.cpp)

add_library(${PROJECT_NAME} STATIC ${LIB_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/h)
