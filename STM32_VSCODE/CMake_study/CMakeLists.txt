# CMake version
cmake_minimum_required(VERSION 3.28.1)

include(GCC.cmake)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/GCC.cmake)

# Khai báo tên dự án
project(embeddat_Project)

# Build the executable based on the source files
add_executable(main main.c)

