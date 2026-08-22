#pragma once
#include <cstdint>
#include <string>
#include <compare>
#include <functional>
#include <iostream>

struct ElementID {
public:
    const uint64_t get_lamport() const;
    const std::string& get_siteid() const;
    std::string to_String() const;
    std::strong_ordering operator<=>(const ElementID& other) const;
    bool operator==(const ElementID& other) const;
    std::string serialize() const;
    static ElementID deserialize(const std::string& serialized_elementID);
    ElementID(uint64_t l, std::string s);

private:
    uint64_t lamport;
    std::string site_id;
};

inline const ElementID ROOT_ID = ElementID(0, "__ROOT__");


template<>
struct std::hash<ElementID> {
    std::size_t operator()(const ElementID& id) const {
        std::size_t h = 0;
        h ^= std::hash<int>{}(id.get_lamport()) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<std::string>{}(id.get_siteid()) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};