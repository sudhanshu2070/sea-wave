cmake_minimum_required(VERSION 3.10)
project(TradeExecutorAPI)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable sanitizers to avoid previous issues
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize=all")

# Find required packages
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PISTACHE libpistache)
else()
    # Fallback: Manually specify Pistache paths if pkg-config is unavailable
    set(PISTACHE_INCLUDE_DIRS /usr/include)
    set(PISTACHE_LIBRARIES pistache)
    link_directories(/usr/lib /usr/local/lib)
endif()

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(${PISTACHE_INCLUDE_DIRS})
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
)

# Header files (for IDEs, not strictly necessary for build)
set(HEADERS
    include/OHLC.h
    include/DataFetcher.h
    include/Renko.h
    include/Ichimoku.h
    include/Trade.h
    include/Strategy.h
    include/BacktestController.h
)

# Add executable
add_executable(trade_api ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(trade_api
    ${PISTACHE_LIBRARIES}
    ${CURL_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

# Set output directory for the executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Optional: If nlohmann/json is installed via package manager
# find_package(nlohmann_json REQUIRED)
# target_link_libraries(trade_api nlohmann_json::nlohmann_json)