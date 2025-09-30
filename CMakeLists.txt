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
include_directories(/usr/include)

# Find dependencies using find_package and pkg-config
find_package(PkgConfig REQUIRED)

# Find Pistache
pkg_check_modules(PISTACHE REQUIRED libpistache)

# Find libcurl
find_package(CURL REQUIRED)

# Find JSON (header-only)
find_package(nlohmann_json 3.2.0 REQUIRED)

# Add executable
add_executable(strategy_api
    src/main.cpp
    src/strategy_api.cpp
    src/strategy_engine.cpp
)

# Include directories from pkg-config
target_include_directories(strategy_api PRIVATE ${PISTACHE_INCLUDE_DIRS})

# Link libraries
target_link_libraries(strategy_api
    ${PISTACHE_LIBRARIES}
    CURL::libcurl
    nlohmann_json::nlohmann_json
    pthread
)

# Compiler flags
target_compile_options(strategy_api PRIVATE 
    ${PISTACHE_CFLAGS_OTHER}
    -Wall
    -Wextra
    -O2
)

# Set C++ standard properties
set_target_properties(strategy_api PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Pistache includes: ${PISTACHE_INCLUDE_DIRS}")
message(STATUS "Pistache libraries: ${PISTACHE_LIBRARIES}")