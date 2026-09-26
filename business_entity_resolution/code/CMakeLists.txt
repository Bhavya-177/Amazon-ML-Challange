cmake_minimum_required(VERSION 3.14)
project(AmazonML_EntityResolution CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -Wall")

add_executable(er_pipeline src/main.cpp)