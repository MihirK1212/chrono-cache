#include "command_handler.h"
#include "resp_serializer.h"

const std::unordered_map<std::string, CommandHandler::CmdFunc> CommandHandler::commands_map = {
    {"PING",    &CommandHandler::cmd_ping},
    {"SET",     &CommandHandler::cmd_set},
    {"GET",     &CommandHandler::cmd_get},
    {"DEL",     &CommandHandler::cmd_del},
    {"EXPIRE",  &CommandHandler::cmd_expire},
    {"PEXPIRE", &CommandHandler::cmd_pexpire},
    {"PERSIST", &CommandHandler::cmd_persist},
    {"PTTL",    &CommandHandler::cmd_pttl},
    {"ZADD",    &CommandHandler::cmd_zadd},
    {"ZSCORE",  &CommandHandler::cmd_zscore},
    {"ZREM",    &CommandHandler::cmd_zrem},
    {"ZRANK",   &CommandHandler::cmd_zrank},
};

CommandHandler::CommandHandler(ChronoCache& cache) : cache(cache) {}

std::string CommandHandler::execute(const RespValue& command) {
    if (command.type != RespType::Array || command.array.empty()) {
        return RespSerializer::serialize(RespValue::error("ERR invalid command format"));
    }

    const std::string& name = command.array[0].str_value; // e.g. "SET"
    auto it = commands_map.find(name);
    if (it == commands_map.end()) {
        return RespSerializer::serialize(RespValue::error("ERR unknown command '" + name + "'"));
    }

    const std::vector<RespValue> args(command.array.begin() + 1, command.array.end());
    return (this->*(it->second))(args);
}

std::string CommandHandler::cmd_ping(const std::vector<RespValue>& args) {
    return "";
}

std::string CommandHandler::cmd_set(const std::vector<RespValue>& args) {
    return "";
}

std::string CommandHandler::cmd_get(const std::vector<RespValue>& args) {
    return "";
}

std::string CommandHandler::cmd_del(const std::vector<RespValue>& args) {
    return "";
}

std::string CommandHandler::cmd_expire(const std::vector<RespValue>& args) {
    return "";
}

std::string CommandHandler::cmd_pexpire(const std::vector<RespValue>& args) {
    return "";
}

std::string CommandHandler::cmd_persist(const std::vector<RespValue>& args) {
    return "";
}

std::string CommandHandler::cmd_pttl(const std::vector<RespValue>& args) {
    return "";
}

std::string CommandHandler::cmd_zadd(const std::vector<RespValue>& args) {
    return "";
}

std::string CommandHandler::cmd_zscore(const std::vector<RespValue>& args) {
    return "";
}

std::string CommandHandler::cmd_zrem(const std::vector<RespValue>& args) {
    return "";
}

std::string CommandHandler::cmd_zrank(const std::vector<RespValue>& args) {
    return "";
}
