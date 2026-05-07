LINUX:

cmake -S . -B build/ && cmake --build build/ && ./build/chrono_cache


cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build && ctest --test-dir build --output-on-failure

./build/sorted_set_tests


WINDOWS:

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build --config Debug && ctest --test-dir build -C Debug --output-on-failure

.\build\Debug\hash_map_tests.exe