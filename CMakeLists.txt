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

# ---- Core library ----
add_library(chrono_cache_core
    src/cpp/chrono_cache.cpp
    src/cpp/sorted_sets/sorted_set.cpp
    src/cpp/sorted_sets/tree.cpp
    src/cpp/kafka/producer.cpp 
    src/cpp/kafka/consumer.cpp
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

target_link_libraries(chrono_cache PRIVATE chrono_cache_core Threads::Threads ${RDKAFKA_LIBRARIES})
target_include_directories(chrono_cache PRIVATE ${RDKAFKA_INCLUDE_DIRS})
target_compile_options(chrono_cache PRIVATE ${RDKAFKA_CFLAGS_OTHER})

target_compile_options(chrono_cache PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-O0 -g>
)
