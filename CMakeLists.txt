cmake_minimum_required(VERSION 3.6)

set(CMAKE_BUILD_TYPE RelWithDebInfo)
project(workflow-extra VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
add_subdirectory(src)
add_subdirectory(example)
