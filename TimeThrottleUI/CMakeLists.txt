cmake_minimum_required(VERSION 3.30)
project(TimeThrottleUI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_executable(TimeThrottleUI WIN32
        main.cpp
        ui_app.cpp
        ui_app.h
        control_pipe_client.cpp
        control_pipe_client.h
)

target_link_libraries(TimeThrottleUI PRIVATE
        user32
        gdi32
        comctl32
)
