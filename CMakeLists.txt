cmake_minimum_required(VERSION 3.21)

project(chrono_cache LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- C++ Standard ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Build type default ----
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---- Compiler warnings ----
add_compile_options(-Wall -Wextra -Wpedantic)

# ---- nlohmann/json (header-only, fetched at configure time) ----
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
    DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(nlohmann_json)

# ---- Core library ----
add_library(chrono_cache_core
    src/cpp/chrono_cache.cpp
    src/cpp/sorted_sets/sorted_set.cpp
    src/cpp/sorted_sets/tree.cpp
    src/cpp/kafka_logging/producer.cpp
    src/cpp/kafka_logging/consumer.cpp
    src/cpp/kafka_logging/event_logger.cpp
    src/cpp/kafka_logging/event_consumer.cpp
)

target_include_directories(chrono_cache_core PUBLIC src/cpp)

target_compile_options(chrono_cache_core PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-O0 -g>
)

# ---- Main executable ----
add_executable(chrono_cache src/cpp/main.cpp)

find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RDKAFKA REQUIRED rdkafka++)

target_link_libraries(chrono_cache_core PUBLIC
    nlohmann_json::nlohmann_json
    ${RDKAFKA_LIBRARIES}
)
target_include_directories(chrono_cache_core PUBLIC ${RDKAFKA_INCLUDE_DIRS})
target_compile_options(chrono_cache_core PUBLIC ${RDKAFKA_CFLAGS_OTHER})

target_link_libraries(chrono_cache PRIVATE chrono_cache_core Threads::Threads)

target_compile_options(chrono_cache PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-O0 -g>
)