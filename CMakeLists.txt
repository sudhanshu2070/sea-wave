cmake_minimum_required(VERSION 3.14)
project(RenkoIchimokuAPI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBCURL REQUIRED libcurl)
find_library(PTHREAD_LIBRARY pthread REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party
    ${LIBCURL_INCLUDE_DIRS}
)

# Sources
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Executable
add_executable(backtest_api ${SOURCES})

# Link libraries
target_link_libraries(backtest_api
    pistache
    ${LIBCURL_LIBRARIES}
    ${PTHREAD_LIBRARY}
    ssl
    crypto
)

# Compile options
target_compile_options(backtest_api PRIVATE -O2 -Wall -Wextra)