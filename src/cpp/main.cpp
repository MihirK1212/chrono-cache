#include <iostream>
#include <string>

#include "store/hash_key.h"

int main() {
    std::cout << "Hello, World!" << std::endl;

    std::string key1 = "test";
    ChronCacheHashKey<std::string> hash_key1(key1);
    std::cout <<"Raw key:"<< hash_key1.get_raw() << " Hashed key: " << hash_key1.get() << std::endl;

    int key2 = 123;
    ChronCacheHashKey<int> hash_key2(key2);
    std::cout <<"Raw key:"<< hash_key2.get_raw() << " Hashed key: " << hash_key2.get() << std::endl;

    // float key3 = 123.456;
    // ChronCacheHashKey<float> hash_key3(key3);
    // std::cout <<"Raw key:"<< hash_key3.get_raw() << " Hashed key: " << hash_key3.get() << std::endl;

    // double key4 = 123.456789;
    // ChronCacheHashKey<double> hash_key4(key4);
    // std::cout <<"Raw key:"<< hash_key4.get_raw() << " Hashed key: " << hash_key4.get() << std::endl;

    // long key5 = 1234567890;
    // ChronCacheHashKey<long> hash_key5(key5);
    // std::cout <<"Raw key:"<< hash_key5.get_raw() << " Hashed key: " << hash_key5.get() << std::endl;

    return 0;
}