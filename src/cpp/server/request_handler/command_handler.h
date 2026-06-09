#ifndef CC_COMMAND_HANDLER_H
#define CC_COMMAND_HANDLER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "resp_types.h"
#include "resp_serializer.h"
#include "../../chrono_cache.h"

class CommandHandler {
public:
    explicit CommandHandler(ChronoCache& cache);

    // Dispatch a parsed RESP command; returns a serialized RESP response string
    std::string execute(const RespValue& command);

private:
    ChronoCache& cache_;

    std::string cmd_ping(const std::vector<RespValue>& args);
    std::string cmd_set(const std::vector<RespValue>& args);
    std::string cmd_get(const std::vector<RespValue>& args);
    std::string cmd_del(const std::vector<RespValue>& args);
    std::string cmd_expire(const std::vector<RespValue>& args);
    std::string cmd_pexpire(const std::vector<RespValue>& args);
    std::string cmd_persist(const std::vector<RespValue>& args);
    std::string cmd_pttl(const std::vector<RespValue>& args);
    std::string cmd_zadd(const std::vector<RespValue>& args);
    std::string cmd_zscore(const std::vector<RespValue>& args);
    std::string cmd_zrem(const std::vector<RespValue>& args);
    std::string cmd_zrank(const std::vector<RespValue>& args);

    using CmdFunc = std::string (CommandHandler::*)(const std::vector<RespValue>&);
    static const std::unordered_map<std::string, CmdFunc> commands_;
};

#endif
