from client import ChronoCacheClient

def main():
    client = ChronoCacheClient("localhost", 2811)
    client.init(with_replay=False)

    print(client.get("key"))
    print(client.set("key", "value"))
    print(client.delete("key"))
    print(client.expire("key", 10))
    print(client.persist("key"))
    print(client.pttl("key"))
    print(client.zadd("key", 1.0, "member"))
    print(client.zscore("key", "member"))
    print(client.zrem("key", "member"))
    print(client.zrank("key", "member"))

if __name__ == "__main__":
    main()