#pragma once
#include <variant>
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <src/character.hpp>
#include <src/elementID.hpp>
#include <src/operation.hpp>

class Document {
public:
    std::string render();
    void visit_visible(const ElementID& node, const std::function<void(const ElementID&)>& visitor) const;
    std::string debug_print();
    std::string visit_for_testing(const ElementID& node);
    void validate() const;
    void apply(const Operation& op);
    std::optional<ElementID> visible_successor(const ElementID node) const;
    std::optional<ElementID> successor(const ElementID node) const;
    const ElementID visible_predecessor(const ElementID node) const;
    const ElementID predecessor(const ElementID node) const;

    std::pair<size_t, size_t> get_line_column(ElementID target) const;
    std::optional<ElementID> get_element_at(size_t target_line, size_t target_column) const;
    std::pair<size_t, size_t> get_cursor_position(const ElementID& anchor) const;
    std::optional<ElementID> get_anchor_at(size_t line, size_t column) const;
    char get_character(ElementID id) const;
    std::optional<size_t> get_line_length(size_t line) const;
    std::unordered_map<ElementID, Character> get_elements() const;
    std::pair<size_t, size_t> clamp_position(size_t requested_line, size_t requested_column) const;
    size_t get_line_count() const;
    Document();

private:
    std::unordered_map<ElementID, Character> elements; // should remove ElementID from Character
    //std::vector<ElementID> ordered_result;
    std::unordered_set<ElementID> buffered_deletes; //can be elementID instead, I don't think it really matters
    std::unordered_map<ElementID, std::vector<InsertOperation>> buffered_inserts;
    std::unordered_map<ElementID, std::vector<ElementID>> children;
    std::unordered_map<ElementID, ElementID> parent_of;
    std::unordered_set<ElementID> buffered_insert_ids;

    void applyInsert(const InsertOperation& oper);
    void applyDelete(const RemoveOperation& oper);

};