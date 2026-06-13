#include "command_handler.h"
#include "resp_serializer.h"

#include <chrono>

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
        return RespSerializer::error("ERR invalid command format");
    }

    const std::string& name = command.array[0].str_value;
    auto it = commands_map.find(name);
    if (it == commands_map.end()) {
        return RespSerializer::error("ERR unknown command '" + name + "'");
    }

    const std::vector<RespValue> args(command.array.begin() + 1, command.array.end());
    return (this->*(it->second))(args);
}

std::string CommandHandler::cmd_ping(const std::vector<RespValue>& args) {
    if (args.empty()) {
        return "+PONG\r\n";
    }
    return RespSerializer::bulk_string(args[0].str_value);
}

std::string CommandHandler::cmd_set(const std::vector<RespValue>& args) {
    if (args.size() < 2) {
        return RespSerializer::error("ERR wrong number of arguments for 'SET' command");
    }

    const std::string& key = args[0].str_value;
    const std::string& value = args[1].str_value;
    std::optional<std::chrono::milliseconds> ttl = std::nullopt;

    if (args.size() >= 4 && (args[2].str_value == "PX" || args[2].str_value == "px")) {
        ttl = std::chrono::milliseconds(std::stoll(args[3].str_value));
    }

    cache.set(key, value, ttl);
    return RespSerializer::ok();
}

std::string CommandHandler::cmd_get(const std::vector<RespValue>& args) {
    if (args.size() != 1) {
        return RespSerializer::error("ERR wrong number of arguments for 'GET' command");
    }

    auto result = cache.get(args[0].str_value);
    if (!result.has_value()) {
        return RespSerializer::null_bulk();
    }
    return RespSerializer::bulk_string(result.value());
}

std::string CommandHandler::cmd_del(const std::vector<RespValue>& args) {
    if (args.empty()) {
        return RespSerializer::error("ERR wrong number of arguments for 'DEL' command");
    }

    int64_t count = 0;
    for (const auto& arg : args) {
        if (cache.del(arg.str_value)) {
            count++;
        }
    }
    return RespSerializer::integer(count);
}

std::string CommandHandler::cmd_expire(const std::vector<RespValue>& args) {
    if (args.size() != 2) {
        return RespSerializer::error("ERR wrong number of arguments for 'EXPIRE' command");
    }

    int64_t ms = std::stoll(args[1].str_value);
    bool ok = cache.expire(args[0].str_value, std::chrono::milliseconds(ms));
    return RespSerializer::integer(ok ? 1 : 0);
}

std::string CommandHandler::cmd_pexpire(const std::vector<RespValue>& args) {
    return cmd_expire(args);
}

std::string CommandHandler::cmd_persist(const std::vector<RespValue>& args) {
    if (args.size() != 1) {
        return RespSerializer::error("ERR wrong number of arguments for 'PERSIST' command");
    }

    bool ok = cache.persist(args[0].str_value);
    return RespSerializer::integer(ok ? 1 : 0);
}

std::string CommandHandler::cmd_pttl(const std::vector<RespValue>& args) {
    if (args.size() != 1) {
        return RespSerializer::error("ERR wrong number of arguments for 'PTTL' command");
    }

    long long ttl = cache.pttl(args[0].str_value);
    return RespSerializer::integer(ttl);
}

std::string CommandHandler::cmd_zadd(const std::vector<RespValue>& args) {
    if (args.size() != 3) {
        return RespSerializer::error("ERR wrong number of arguments for 'ZADD' command");
    }

    const std::string& key = args[0].str_value;
    double score = std::stod(args[1].str_value);
    const std::string& member = args[2].str_value;

    bool added = cache.zadd(key, score, member);
    return RespSerializer::integer(added ? 1 : 0);
}

std::string CommandHandler::cmd_zscore(const std::vector<RespValue>& args) {
    if (args.size() != 2) {
        return RespSerializer::error("ERR wrong number of arguments for 'ZSCORE' command");
    }

    auto score = cache.zscore(args[0].str_value, args[1].str_value);
    if (!score.has_value()) {
        return RespSerializer::null_bulk();
    }
    return RespSerializer::bulk_string(std::to_string(score.value()));
}

std::string CommandHandler::cmd_zrem(const std::vector<RespValue>& args) {
    if (args.size() != 2) {
        return RespSerializer::error("ERR wrong number of arguments for 'ZREM' command");
    }

    bool removed = cache.zrem(args[0].str_value, args[1].str_value);
    return RespSerializer::integer(removed ? 1 : 0);
}

std::string CommandHandler::cmd_zrank(const std::vector<RespValue>& args) {
    if (args.size() != 2) {
        return RespSerializer::error("ERR wrong number of arguments for 'ZRANK' command");
    }

    auto rank = cache.zrank(args[0].str_value, args[1].str_value);
    if (!rank.has_value()) {
        return RespSerializer::null_bulk();
    }
    return RespSerializer::integer(rank.value());
}
