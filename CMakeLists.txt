cmake_minimum_required(VERSION 3.12)
project(BacktestAPI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(PISTACHE REQUIRED libpistache)
pkg_check_modules(NLOHMANN_JSON REQUIRED nlohmann_json)
pkg_check_modules(LIBCURL REQUIRED libcurl)

# Include directories
include_directories(${PISTACHE_INCLUDE_DIRS})
include_directories(${NLOHMANN_JSON_INCLUDE_DIRS})
include_directories(${LIBCURL_INCLUDE_DIRS})

# Add executable
add_executable(backtest_api main.cpp)

# Link libraries
target_link_libraries(backtest_api ${PISTACHE_LIBRARIES})
target_link_libraries(backtest_api ${NLOHMANN_JSON_LIBRARIES})
target_link_libraries(backtest_api ${LIBCURL_LIBRARIES})
target_link_libraries(backtest_api pthread)