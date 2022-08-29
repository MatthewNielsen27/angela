cmake_minimum_required(VERSION 3.18)
project(angela)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL REQUIRED)

add_library(angela
        include/angela/Checksum.hpp
        include/angela/Merkle.hpp
        src/Merkle.cpp
)

target_include_directories(angela PUBLIC include)

target_link_libraries(angela
    PUBLIC
        OpenSSL::Crypto
)

add_executable(angela_main
        main.cpp
)

target_link_libraries(angela_main
    angela
)

