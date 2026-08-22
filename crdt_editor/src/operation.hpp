#pragma once
#include <iostream>
#include <string>
#include <ranges>
#include <variant>
#include <src/character.hpp>
#include <src/elementID.hpp>


class InsertOperation{
public:

    const ElementID& get_id() const;
    const char get_char() const;
    const ElementID& get_parent() const;

    std::string serialize() const;
    static InsertOperation deserialize(const std::string& serialized_elementID);
    InsertOperation(ElementID p, ElementID id, char c);

private:
    ElementID parent;
    ElementID ident;
    char ch;
};

class RemoveOperation{
public:

    const ElementID& get_target() const;
    RemoveOperation(ElementID target);
    std::string serialize() const;
    static RemoveOperation deserialize(const std::string& serialized_elementID);

private:
    ElementID the_target;
};

using Operation = std::variant<InsertOperation, RemoveOperation>;


namespace operation_serializer {
    std::string serialize(const Operation& op);
    Operation deserialize(const std::string& s);
}