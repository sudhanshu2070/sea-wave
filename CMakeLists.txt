cmake_minimum_required(VERSION 3.14)
project(StrategyBacktestAPI VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(include)
include_directories(src/third_party)

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(PISTACHE REQUIRED pistache)
pkg_check_modules(LIBCURL REQUIRED libcurl)
pkg_check_modules(NLOHMANN_JSON REQUIRED nlohmann_json)

# Add executable
add_executable(strategy_api
    src/main.cpp
    src/strategy_api.cpp
    src/strategy_engine.cpp
)

# Link libraries
target_link_libraries(strategy_api
    ${PISTACHE_LIBRARIES}
    ${LIBCURL_LIBRARIES}
    pthread
    stdc++fs
)

# Compiler flags
target_compile_options(strategy_api PRIVATE 
    ${PISTACHE_CFLAGS_OTHER}
    -Wall
    -Wextra
    -O2
)

# Installation
install(TARGETS strategy_api DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)