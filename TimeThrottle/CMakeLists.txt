cmake_minimum_required(VERSION 3.30)
project(TimeThrottle)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Глобальные определения
add_definitions(-DUNICODE -D_UNICODE)

# Подключаем nlohmann_json через FetchContent (лучше делать до target)
include(FetchContent)
FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Список исходных файлов
add_executable(${PROJECT_NAME}
        main.cpp
        monitor.cpp
        monitor.h
        net_delay.cpp
        net_delay.h
        utils.cpp
        utils.h
        app_config.cpp
        app_config.h
        proxy_server.cpp
        proxy_server.h
        logger.cpp
        logger.h
        process_guard.cpp
        process_guard.h
)

# Пути к заголовкам (spdlog и твои либы)
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/lib
        ${CMAKE_SOURCE_DIR}/lib/spdlog/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/WinDivert.lib
        nlohmann_json::nlohmann_json
        psapi
        kernel32
        user32
        advapi32
        ws2_32
        iphlpapi
)