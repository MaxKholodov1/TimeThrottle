cmake_minimum_required(VERSION 3.30)
project(WatchDog)

if(MSVC)
    add_compile_options(/utf-8)
endif()


set(CMAKE_CXX_STANDARD 20)

add_executable(WatchDog main.cpp
        process_guard.cpp
        process_guard.h)
include_directories(lib/spdlog/include)

