
#include <src/operation.hpp>

const ElementID& InsertOperation::get_id() const {return ident;}

const char InsertOperation::get_char() const {return ch;}

const ElementID& InsertOperation::get_parent() const {return parent;}

std::string InsertOperation::serialize() const {
    std::string res = "INSERT|";
    res.append(get_parent().serialize());
    res.push_back('|');
    res.append(get_id().serialize());
    res.push_back('|');
    res.append(std::to_string(static_cast<unsigned int>(static_cast<unsigned char>(ch))));
    return res;
}

InsertOperation InsertOperation::deserialize(const std::string& serialized_elementID) {
    auto split_view = std::views::split(serialized_elementID, '|');
    std::vector<std::string> tokens;
    for (auto&& subrange : split_view) { // the substrings; the middle two are the elementID
        tokens.emplace_back(subrange.begin(), subrange.end());
    }

    if (tokens.size() != 4) { // need to fix this once I settle on serialization structure
        throw std::invalid_argument("serialized insert operation does not have a singleton character.");
    }

    ElementID elemID1 = ElementID::deserialize(tokens.at(1));
    ElementID elemID2 = ElementID::deserialize(tokens.at(2));

    unsigned int value = std::stoul(tokens.at(3));

    if (value > 255) {
        throw std::invalid_argument("serialized character value is out of range");
    }

    char c = static_cast<char>(value);

    return InsertOperation(elemID1, elemID2, c);
}

InsertOperation::InsertOperation(ElementID p, ElementID id, char c): ident(id), ch(c), parent(p) {}



const ElementID& RemoveOperation::get_target() const {return the_target;}

RemoveOperation::RemoveOperation(ElementID target): the_target(target) {}

std::string RemoveOperation::serialize() const {
    std::string res = "REMOVE|";
    res.append(the_target.serialize());
    return res;
}

RemoveOperation RemoveOperation::deserialize(const std::string& serialized_elementID) {
    auto split_view = std::views::split(serialized_elementID, '|');
    std::vector<std::string> tokens;
    for (auto&& subrange : split_view) { // the substrings; the later two are the elementID
        tokens.emplace_back(subrange.begin(), subrange.end());
    }

    if (tokens.size() != 2) {
        throw std::invalid_argument("serialized insert operation is incorrect.");
    }
    ElementID elemID = ElementID::deserialize(tokens.at(1));

    return RemoveOperation(elemID);
}




namespace operation_serializer {

    std::string serialize(const Operation& op) {

    return std::visit([&](const auto& operation) { 
        return operation.serialize();
    }, op);
    }

    Operation deserialize(const std::string& s) {
        if (s.length() != 0) {
            if (s.starts_with("INSERT|")) {
                return InsertOperation::deserialize(s);
            } else if (s.starts_with("REMOVE|")) {
                return RemoveOperation::deserialize(s);
            } else {
                throw std::invalid_argument("serialized operation has failed");
            }
        } else {
            throw std::invalid_argument("nothing passed to deserialize.");
        }
    }
}