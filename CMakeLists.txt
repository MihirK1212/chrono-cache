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

# ---- Compiler warnings (good for engine code) ----
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
)

# ---- Source files ----
file(GLOB_RECURSE SRC_FILES
    src/cpp/*.cpp
)

# ---- Executable ----
add_executable(chrono_cache ${SRC_FILES})

# ---- Include dirs ----
target_include_directories(chrono_cache PRIVATE
    src/cpp
)

# ---- Threads (future multithreading / RL workers) ----
find_package(Threads REQUIRED)
target_link_libraries(chrono_cache PRIVATE Threads::Threads)

# ---- Optimization presets ----
target_compile_options(chrono_cache PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Debug>:-O0 -g>
)