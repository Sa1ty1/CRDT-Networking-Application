#include <src/elementID.hpp>


const uint64_t ElementID::get_lamport() const {return lamport;}
const std::string& ElementID::get_siteid() const {return site_id;}

std::string ElementID::to_String() const {
    return "<" + std::to_string(lamport) + ", " + site_id + ">";
}

std::strong_ordering ElementID::operator<=>(const ElementID& other) const {
    if (auto cmp = lamport <=> other.lamport; cmp != 0) return cmp;
    return site_id <=> other.site_id;
}

bool ElementID::operator==(const ElementID& other) const {
    return (lamport == other.lamport && site_id == other.site_id);
}

std::string ElementID::serialize() const { // 5,A
    return std::to_string(lamport) + ',' + site_id;
}

ElementID ElementID::deserialize(const std::string& serialized_elementID) {
    size_t comma = serialized_elementID.find(',');
    if (comma == std::string::npos) {
        std::string message = "Malformed PositionComponent: " + serialized_elementID;
        throw std::invalid_argument(message);
    }
    return ElementID(std::stoull(serialized_elementID.substr(0, comma)), serialized_elementID.substr(comma + 1));
}

ElementID::ElementID(uint64_t l, std::string s) : lamport(l), site_id(s) {}
