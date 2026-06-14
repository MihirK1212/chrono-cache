#include <string>
#include <cstring>

#include "chrono_cache.h"
#include "server.h"


int main(int argc, char* argv[]) 
{
    bool disable_kafka = false;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--no-kafka") == 0) {
            disable_kafka = true;
        }
    }

    CacheConfig config(
        "localhost:9092",
        "chrono-events",
        disable_kafka
    );
    ChronoCache cache(config);

    ChronoCacheServer srv({2811, 0}, cache);
    srv.start();
    srv.wait();

    srv.stop();
    
    return 0;
}