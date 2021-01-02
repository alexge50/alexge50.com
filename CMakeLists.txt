cmake_minimum_required(VERSION 3.9)
project(background)

set(CMAKE_CXX_STANDARD 20)

#set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

find_package(SDL2 REQUIRED)

add_executable(background main.cpp)
target_link_libraries(background SDL2::SDL2)
