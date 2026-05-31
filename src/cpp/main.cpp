#include <string>

#include "chrono_cache.h"
#include "server/server.h"


int main() {

    CacheConfig config("localhost:9092", "chrono-events", false);
    ChronoCache cache(config);

    ChronoCacheServer srv(cache);
    srv.start();

    // CacheConfig config("localhost:9092", "chrono-events", false);

    // ChronoCacheCLI cli(config);
    // cli.run();

    return 0;
}