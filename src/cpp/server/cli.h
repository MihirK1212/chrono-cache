#ifndef CHRONO_CACHE_CLI_H
#define CHRONO_CACHE_CLI_H

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../cache/cache_config.h"
#include "../cache/chrono_cache.h"

class ChronoCacheCLI {
    const CacheConfig& config;
    std::unique_ptr<ChronoCache> cache;

    static std::vector<std::string> tokenize(const std::string& line) {
        std::vector<std::string> tokens;
        std::string token;
        bool in_quotes = false;
        char quote_char = 0;

        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (in_quotes) {
                if (c == quote_char) {
                    in_quotes = false;
                    tokens.push_back(token);
                    token.clear();
                } else {
                    token += c;
                }
            } else if (c == '"' || c == '\'') {
                in_quotes = true;
                quote_char = c;
            } else if (std::isspace(static_cast<unsigned char>(c))) {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
            } else {
                token += c;
            }
        }
        if (!token.empty()) tokens.push_back(token);
        return tokens;
    }

    static std::string to_upper(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        return s;
    }

    bool is_ready() const {
        return cache != nullptr && cache->is_accepting_ops();
    }

    bool do_init(bool with_replay) {
        cache = std::make_unique<ChronoCache>(config);
        try {
            return cache->init(with_replay);
        } catch (const std::exception&) {
            cache.reset();
            throw;
        }
    }

    void print_help() const {
        std::cout <<
            "\nAvailable commands:\n"
            "  Key-Value:\n"
            "    GET <key>                      get the value of a key\n"
            "    SET <key> <value> [ttl_ms]     set key to value, optional TTL in ms\n"
            "    DEL <key>                      delete a key\n"
            "    EXPIRE <key> <ttl_ms>          set TTL (ms) on an existing key\n"
            "    PERSIST <key>                  remove TTL from a key\n"
            "    TTL <key>                      remaining TTL in ms (-1=no TTL, -2=missing)\n"
            "\n"
            "  Sorted Sets:\n"
            "    ZADD <key> <score> <member>    add/update member with score in sorted set\n"
            "    ZSCORE <key> <member>          get score of a member\n"
            "    ZREM <key> <member>            remove member from sorted set\n"
            "    ZRANK <key> <member>           0-based rank of member (ascending)\n"
            "\n"
            "  Lifecycle:\n"
            "    INIT [false|true]              initialise the cache:\n"
            "                                     false (default) - start empty, no replay\n"
            "                                     true            - rebuild from Kafka event log\n"
            "    RESET                          destroy current state; requires INIT to continue\n"
            "\n"
            "  Utility:\n"
            "    PING                           returns PONG if cache is ready\n"
            "    INFO                           show configuration and status\n"
            "    HELP                           show this help message\n"
            "    QUIT / EXIT                    exit the CLI\n"
            "\n"
            "  Tips:\n"
            "    Wrap values with spaces in quotes: SET greeting \"hello world\"\n"
            "    TTL values are always in milliseconds.\n\n";
    }

    void cmd_get(const std::vector<std::string>& args) {
        if (args.size() != 2) { std::cout << "(error) Usage: GET <key>\n"; return; }
        try {
            auto val = cache->get(args[1]);
            std::cout << (val.has_value() ? ("\"" + *val + "\"") : "(nil)") << "\n";
        } catch (const std::exception& e) {
            std::cout << "(error) " << e.what() << "\n";
        }
    }

    void cmd_set(const std::vector<std::string>& args) {
        if (args.size() < 3 || args.size() > 4) {
            std::cout << "(error) Usage: SET <key> <value> [ttl_ms]\n"; return;
        }
        try {
            bool ok;
            if (args.size() == 4) {
                long long ms = std::stoll(args[3]);
                if (ms <= 0) { std::cout << "(error) TTL must be > 0\n"; return; }
                ok = cache->set(args[1], args[2], std::chrono::milliseconds(ms));
            } else {
                ok = cache->set(args[1], args[2]);
            }
            std::cout << (ok ? "OK" : "(error) SET failed") << "\n";
        } catch (const std::exception& e) {
            std::cout << "(error) " << e.what() << "\n";
        }
    }

    void cmd_del(const std::vector<std::string>& args) {
        if (args.size() != 2) { std::cout << "(error) Usage: DEL <key>\n"; return; }
        try {
            bool removed = cache->del(args[1]);
            std::cout << (removed ? "(integer) 1" : "(integer) 0") << "\n";
        } catch (const std::exception& e) {
            std::cout << "(error) " << e.what() << "\n";
        }
    }

    void cmd_expire(const std::vector<std::string>& args) {
        if (args.size() != 3) { std::cout << "(error) Usage: EXPIRE <key> <ttl_ms>\n"; return; }
        try {
            long long ms = std::stoll(args[2]);
            if (ms <= 0) { std::cout << "(error) TTL must be > 0\n"; return; }
            bool ok = cache->expire(args[1], std::chrono::milliseconds(ms));
            std::cout << (ok ? "(integer) 1" : "(integer) 0") << "\n";
        } catch (const std::exception& e) {
            std::cout << "(error) " << e.what() << "\n";
        }
    }

    void cmd_persist(const std::vector<std::string>& args) {
        if (args.size() != 2) { std::cout << "(error) Usage: PERSIST <key>\n"; return; }
        try {
            bool ok = cache->persist(args[1]);
            std::cout << (ok ? "(integer) 1" : "(integer) 0") << "\n";
        } catch (const std::exception& e) {
            std::cout << "(error) " << e.what() << "\n";
        }
    }

    void cmd_ttl(const std::vector<std::string>& args) {
        if (args.size() != 2) { std::cout << "(error) Usage: TTL <key>\n"; return; }
        try {
            long long ms = cache->pttl(args[1]);
            if      (ms == -2) std::cout << "(integer) -2  # key does not exist\n";
            else if (ms == -1) std::cout << "(integer) -1  # key has no TTL (persistent)\n";
            else               std::cout << "(integer) " << ms << "  # ms remaining\n";
        } catch (const std::exception& e) {
            std::cout << "(error) " << e.what() << "\n";
        }
    }

    void cmd_zadd(const std::vector<std::string>& args) {
        if (args.size() != 4) { std::cout << "(error) Usage: ZADD <key> <score> <member>\n"; return; }
        try {
            double score = std::stod(args[2]);
            bool ok = cache->zadd(args[1], score, args[3]);
            std::cout << (ok ? "(integer) 1" : "(integer) 0") << "\n";
        } catch (const std::exception& e) {
            std::cout << "(error) " << e.what() << "\n";
        }
    }

    void cmd_zscore(const std::vector<std::string>& args) {
        if (args.size() != 3) { std::cout << "(error) Usage: ZSCORE <key> <member>\n"; return; }
        try {
            auto score = cache->zscore(args[1], args[2]);
            if (score.has_value()) std::cout << score.value() << "\n";
            else                   std::cout << "(nil)\n";
        } catch (const std::exception& e) {
            std::cout << "(error) " << e.what() << "\n";
        }
    }

    void cmd_zrem(const std::vector<std::string>& args) {
        if (args.size() != 3) { std::cout << "(error) Usage: ZREM <key> <member>\n"; return; }
        try {
            bool ok = cache->zrem(args[1], args[2]);
            std::cout << (ok ? "(integer) 1" : "(integer) 0") << "\n";
        } catch (const std::exception& e) {
            std::cout << "(error) " << e.what() << "\n";
        }
    }

    void cmd_zrank(const std::vector<std::string>& args) {
        if (args.size() != 3) { std::cout << "(error) Usage: ZRANK <key> <member>\n"; return; }
        try {
            auto rank = cache->zrank(args[1], args[2]);
            if (rank.has_value()) std::cout << "(integer) " << rank.value() << "\n";
            else                  std::cout << "(nil)\n";
        } catch (const std::exception& e) {
            std::cout << "(error) " << e.what() << "\n";
        }
    }

    void cmd_info() const {
        const char* status =
            !cache                      ? "uninitialized" :
            cache->is_accepting_ops()   ? "ready"         :
                                          "not ready";
        std::cout << "# chrono-cache\n"
                  << "status:        " << status                                                    << "\n"
                  << "kafka_brokers: " << config.kafka_brokers                                      << "\n"
                  << "kafka_topic:   " << config.kafka_cache_events_topic                           << "\n"
                  << "event_logging: " << (config.disable_event_logging ? "disabled" : "enabled")  << "\n";
    }

    bool dispatch(const std::vector<std::string>& args) {
        const std::string cmd = to_upper(args[0]);

        if (cmd == "QUIT" || cmd == "EXIT") { std::cout << "Bye!\n"; return false; }
        if (cmd == "PING") {
            if (is_ready()) std::cout << "PONG\n";
            else            std::cout << "(error) Cache not ready — run INIT first\n";
            return true;
        }
        if (cmd == "HELP")  { print_help();           return true; }
        if (cmd == "INFO")  { cmd_info();             return true; }

        if (cmd == "INIT") {
            if (is_ready()) {
                std::cout << "(error) Cache is already running. Use RESET first.\n";
                return true;
            }
            bool with_replay = false;
            if (args.size() >= 2) {
                const std::string flag = to_upper(args[1]);
                if (flag == "TRUE")       { with_replay = true;  }
                else if (flag == "FALSE") { with_replay = false; }
                else {
                    std::cout << "(error) Usage: INIT [false|true]\n";
                    return true;
                }
            }
            try {
                if (with_replay) std::cout << "Replaying events from Kafka log...\n";
                do_init(with_replay);
                std::cout << (with_replay ? "OK  (cache rebuilt from event log)\n"
                                          : "OK  (empty cache ready — no replay)\n");
            } catch (const std::exception& e) {
                std::cout << "(error) " << e.what() << "\n";
            }
            return true;
        }

        if (cmd == "RESET") {
            cache.reset();
            std::cout << "OK  (cache destroyed — type INIT to continue)\n";
            return true;
        }

        if (!is_ready()) {
            std::cout << "(error) Cache is not initialised. Run INIT or INIT true first.\n";
            return true;
        }

        if (cmd == "GET")     { cmd_get(args);    return true; }
        if (cmd == "SET")     { cmd_set(args);    return true; }
        if (cmd == "DEL")     { cmd_del(args);    return true; }
        if (cmd == "EXPIRE")  { cmd_expire(args); return true; }
        if (cmd == "PERSIST") { cmd_persist(args);return true; }
        if (cmd == "TTL")     { cmd_ttl(args);    return true; }
        if (cmd == "ZADD")    { cmd_zadd(args);   return true; }
        if (cmd == "ZSCORE")  { cmd_zscore(args); return true; }
        if (cmd == "ZREM")    { cmd_zrem(args);   return true; }
        if (cmd == "ZRANK")   { cmd_zrank(args);  return true; }

        std::cout << "(error) Unknown command '" << args[0]
                  << "'. Type HELP for a list of commands.\n";
        return true;
    }

    public:
    explicit ChronoCacheCLI(const CacheConfig& config) : config(config) {}

    void run() {
        std::cout << "chrono-cache CLI  (type HELP for commands, QUIT to exit)\n"
                  << "Run INIT to start empty, or INIT true to restore from the Kafka event log.\n\n";

        std::string line;
        while (true) {
            std::cout << "chrono-cache> ";
            std::cout.flush();

            if (!std::getline(std::cin, line)) break;

            std::vector<std::string> args = tokenize(line);
            if (args.empty()) continue;

            if (!dispatch(args)) break;
        }
    }
};

#endif
