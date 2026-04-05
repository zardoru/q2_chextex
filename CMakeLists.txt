cmake_minimum_required(VERSION 3.17)
project(q2_chextex)

set(CMAKE_CXX_STANDARD 23)

find_package(Boost REQUIRED NO_MODULE COMPONENTS program_options)

add_executable(q2_chextex
        chextex.cpp
        bsp/bsp.cpp
        fs/pak.cpp
        fs/q2fs.h
        fs/q2fs.cpp
        shared.h
        parse.cpp)
target_link_libraries(q2_chextex PRIVATE Boost::headers Boost::program_options)
