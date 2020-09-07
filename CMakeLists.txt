cmake_minimum_required(VERSION 3.1)
project(lsl_bridge CXX)
add_compile_options(-D HASBLUETOOTH)
include_directories(./include)
link_directories(./lib)
add_executable(lsl_bridge main.cpp bitalino.cpp)
target_link_libraries(lsl_bridge liblsl.so bluetooth)

