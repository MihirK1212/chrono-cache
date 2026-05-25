#ifndef CACHE_STATE_H
#define CACHE_STATE_H

enum class ChronoCacheState {
    UNINITIALIZED, // completely uninitialized, reject all ops
    REPLAYING, // replay ongoing, accept all ops but do not log events
    REPLAY_FAILED, // replay failed, reject all ops (until force moved to ready)
    REPLAY_SUCCESS, // replay success, reject all ops (unilt moved to ready)
    READY // all success, ready to accept operations
};

#endif