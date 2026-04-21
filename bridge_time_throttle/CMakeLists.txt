cmake_minimum_required(VERSION 3.30)
project(bridge_time_throttle)

if(MSVC)
    add_compile_options(/utf-8)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_CXX_STANDARD 20)

add_executable(bridge_time_throttle main.cpp)
include_directories(lib/spdlog/include)

