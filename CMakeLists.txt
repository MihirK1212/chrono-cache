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

find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RDKAFKA REQUIRED rdkafka++)

# ---- Cache library ----
add_library(cache
    src/cpp/cache/chrono_cache.cpp
    src/cpp/cache/sorted_sets/sorted_set.cpp
    src/cpp/cache/sorted_sets/tree.cpp
    src/cpp/cache/kafka_logging/producer.cpp
    src/cpp/cache/kafka_logging/consumer.cpp
    src/cpp/cache/kafka_logging/event_logger.cpp
    src/cpp/cache/kafka_logging/event_consumer.cpp
)

target_include_directories(cache PUBLIC src/cpp/cache)

target_compile_options(cache PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-O0 -g>
)

target_link_libraries(cache PUBLIC
    nlohmann_json::nlohmann_json
    ${RDKAFKA_LIBRARIES}
)
target_include_directories(cache PUBLIC ${RDKAFKA_INCLUDE_DIRS})
target_compile_options(cache PUBLIC ${RDKAFKA_CFLAGS_OTHER})

# ---- Server library ----
add_library(server
    src/cpp/server/server.cpp
    src/cpp/server/worker.cpp
    src/cpp/server/event_loop.cpp
    src/cpp/server/socket_utils.cpp
    src/cpp/server/request_handler/resp_value.cpp
    src/cpp/server/request_handler/utils.cpp
    src/cpp/server/request_handler/resp_parser.cpp
    src/cpp/server/request_handler/resp_serializer.cpp
    src/cpp/server/request_handler/command_handler.cpp
)

target_include_directories(server PUBLIC src/cpp/server)

target_compile_options(server PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-O0 -g>
)

target_link_libraries(server PUBLIC cache)

# ---- Main executable ----
add_executable(chrono_cache src/cpp/main.cpp)

target_link_libraries(chrono_cache PRIVATE server Threads::Threads)

target_compile_options(chrono_cache PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-O0 -g>
)
