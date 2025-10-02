# CMakeLists.txt
cmake_minimum_required(VERSION 3.14)
project(EthRenkoBacktestAPI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

# Find dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(PISTACHE REQUIRED pistache)
find_package(CURL REQUIRED)

# Include directories
include_directories(${PISTACHE_INCLUDE_DIRS})
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add executable
add_executable(backtest_api src/api.cpp)

# Link libraries
target_link_libraries(backtest_api 
    ${PISTACHE_LIBRARIES}
    ${CURL_LIBRARIES}
    pthread
)