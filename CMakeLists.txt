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
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ── Main executable ────────────────────────────────────────────────────────────

file(GLOB_RECURSE SRC_FILES src/cpp/*.cpp)
list(FILTER SRC_FILES EXCLUDE REGEX ".*/tests/.*")

add_executable(chrono_cache ${SRC_FILES})

target_include_directories(chrono_cache PRIVATE src/cpp)

find_package(Threads REQUIRED)
target_link_libraries(chrono_cache PRIVATE Threads::Threads)

if(MSVC)
    target_compile_options(chrono_cache PRIVATE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Od /Zi>
    )
else()
    target_compile_options(chrono_cache PRIVATE
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-O0 -g>
    )
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────

enable_testing()

file(GLOB TEST_FILES src/cpp/store/tests/*.cpp)

add_executable(hash_map_tests ${TEST_FILES})

target_include_directories(hash_map_tests PRIVATE src/cpp)

if(MSVC)
    target_compile_options(hash_map_tests PRIVATE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Od /Zi>
    )
else()
    target_compile_options(hash_map_tests PRIVATE
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-O0 -g>
    )
endif()

add_test(NAME HashMapTests COMMAND hash_map_tests)
