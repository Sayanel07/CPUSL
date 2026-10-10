cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Test)

file(GLOB TEST_SOURCES
        src/h/*.h
        src/cpp/*.cpp)

add_executable(${PROJECT_NAME} WIN32 ${TEST_SOURCES})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})