#include <string>

#include "chrono_cache.h"
#include "server.h"


int main() {

    CacheConfig config("localhost:9092", "chrono-events", false);
    ChronoCache cache(config);

    ChronoCacheServer srv({2811, 0}, cache);
    srv.start();
    srv.wait();

    srv.stop();
    
    return 0;
}