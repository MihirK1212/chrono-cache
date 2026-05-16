LINUX:

cmake -S . -B build/ && cmake --build build/ && ./build/chrono_cache


cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build && ctest --test-dir build --output-on-failure

./build/chrono_cache_tests


WINDOWS:

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build --config Debug && ctest --test-dir build -C Debug --output-on-failure

.\build\Debug\hash_map_tests.exe


KAFKA:

**Terminal 1 -> start server**
cd kafka_2.13-4.2.0

# Only needed once (first boot) — skip if already formatted
KAFKA_CLUSTER_ID="$(bin/kafka-storage.sh random-uuid)"
bin/kafka-storage.sh format --standalone -t $KAFKA_CLUSTER_ID -c config/server.properties

bin/kafka-server-start.sh config/server.properties

**Terminal 2 -> create topics**
cd kafka_2.13-4.2.0

create topic:
bin/kafka-topics.sh \
  --create \
  --topic chrono-events \
  --partitions 4 \
  --replication-factor 1 \
  --config cleanup.policy=compact \
  --config retention.ms=-1 \
  --config min.cleanable.dirty.ratio=0.1 \
  --bootstrap-server localhost:9092


verify:
bin/kafka-topics.sh \
  --describe \
  --topic chrono-events \
  --bootstrap-server localhost:9092


delete topic:
bin/kafka-topics.sh \
  --delete \
  --topic chrono-events \
  --bootstrap-server localhost:9092

PROTOBUF:
protoc -I=/home/mihir/projects/chrono-cache --cpp_out=/home/mihir/projects/chrono-cache/src/cpp/proto_schema /home/mihir/projects/chrono-cache/proto/cache_event.proto

