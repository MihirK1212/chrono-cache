cmake -S . -B build/ && cmake --build build/ && ./build/chrono_cache




cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build && ctest --test-dir build --output-on-failure