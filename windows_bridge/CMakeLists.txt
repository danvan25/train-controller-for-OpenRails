cmake_minimum_required(VERSION 3.20)

project(openrails_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(openrails_bridge
    src/main.cpp
    src/serial_port.cpp
    src/message_parser.cpp
    src/openrails_client.cpp
)

target_include_directories(openrails_bridge
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(openrails_bridge
    PRIVATE
        winhttp
)