#include <iostream>
#include <string>

#include "store/hash_key.h"
#include "store/hash_map.h"

void run_hash_key_examples() {
    std::cout << "\n--- Hash Key Examples ---" << std::endl;

    std::string key1 = "test";
    ChronCacheHashKey<std::string> hash_key1(key1);
    std::cout <<"Raw key:"<< hash_key1.get_raw() << " Hashed key: " << hash_key1.get() << std::endl;

    int key2 = 123;
    ChronCacheHashKey<int> hash_key2(key2);
    std::cout <<"Raw key:"<< hash_key2.get_raw() << " Hashed key: " << hash_key2.get() << std::endl;

    float key3 = 123.456;
    ChronCacheHashKey<float> hash_key3(key3);
    std::cout <<"Raw key:"<< hash_key3.get_raw() << " Hashed key: " << hash_key3.get() << std::endl;

    double key4 = 123.456789;
    ChronCacheHashKey<double> hash_key4(key4);
    std::cout <<"Raw key:"<< hash_key4.get_raw() << " Hashed key: " << hash_key4.get() << std::endl;

    long key5 = 1234567890;
    ChronCacheHashKey<long> hash_key5(key5);
    std::cout <<"Raw key:"<< hash_key5.get_raw() << " Hashed key: " << hash_key5.get() << std::endl;
}

void run_hash_map_examples() {
    std::cout << "\n--- Hash Map Examples ---" << std::endl;
    ChronCacheHashMap<std::string, std::string> string_map(16);

    // Set examples
    string_map.set("key1", "value1");
    string_map.set("key2", "value2");
    string_map.set("key3", "value3");
    std::cout << "Set key1, key2, key3" << std::endl;

    // Get examples
    try {
        std::cout << "Get key1: " << string_map.get("key1") << std::endl;
        std::cout << "Get key2: " << string_map.get("key2") << std::endl;
        std::cout << "Get key3: " << string_map.get("key3") << std::endl;
        std::cout << "Get non-existent key4: ";
        std::cout << string_map.get("key4") << std::endl; // This should throw an exception
    } catch (const std::runtime_error& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }

    // Delete examples
    std::cout << "\n--- Hash Map Delete Examples ---" << std::endl;
    string_map.set("del_key1", "del_value1");
    string_map.set("del_key2", "del_value2");
    std::cout << "Set del_key1, del_key2" << std::endl;

    try {
        std::cout << "Before delete - Get del_key1: " << string_map.get("del_key1") << std::endl;
    } catch (const std::runtime_error& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }
    
    string_map.remove("del_key1");
    std::cout << "Removed del_key1" << std::endl;

    try {
        std::cout << "After delete - Get del_key1: ";
        std::cout << string_map.get("del_key1") << std::endl; // This should throw an exception
    } catch (const std::runtime_error& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
    }

    // Attempt to delete a non-existent key
    std::cout << "Attempting to remove non-existent key: non_existent_key. Success: " << string_map.remove("non_existent_key") << std::endl;
}

int main() {
    std::cout << "Hello, World!" << std::endl;

    run_hash_key_examples();
    run_hash_map_examples();

    return 0;
}