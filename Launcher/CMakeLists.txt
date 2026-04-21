cmake_minimum_required(VERSION 3.20)
cmake_minimum_required(VERSION 3.30)
project(Launcher)

if(MSVC)
    add_compile_options(/utf-8)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_CXX_STANDARD 20)

add_executable(Launcher
        main.cpp
        functions.cpp
)

target_link_libraries(Launcher PRIVATE Advapi32.lib User32.lib Shell32.lib)