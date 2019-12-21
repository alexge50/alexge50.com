cmake_minimum_required(VERSION 3.9)
project(background)

find_package(SDL2 REQUIRED)

add_executable(background main.cpp)
target_link_libraries(background SDL2::SDL2)
