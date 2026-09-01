#pragma once
#include <iostream>
#include <src/elementID.hpp>
#include <src/document.hpp>


class Cursor {
public:
    const ElementID get_anchor() const;

    void set_anchor(ElementID new_anchor);

    void update_on_insert(const Document& doc, const InsertOperation& oper);

    void update_on_delete(const Document& doc, const RemoveOperation& oper);

    void update(const Document& doc, const Operation& oper);

    void update_on_remote(const Document& doc, const Operation& oper);

    std::pair<size_t, size_t> get_line_column(const Document& doc) const;
    bool has_desired_column() const;
    void set_desired_column(size_t column);
    std::optional<size_t> get_desired_column() const;

    void update_desired_column(const Document& document);

    void set_position(ElementID new_anchor, const Document& doc);

    std::optional<DocumentRange> normalized_range(const Document& doc) const;

    bool has_selection() const;
    const ElementID& get_selection_anchor() const;

    void start_selection();
    void clear_selection();

    Cursor (ElementID p);

private:
    ElementID anchor;
    std::optional<size_t> desired_column;
    std::optional<ElementID> selection_anchor;
};