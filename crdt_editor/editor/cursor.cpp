#include <editor/cursor.hpp>


const ElementID Cursor::get_anchor() const {return anchor;}

void Cursor::set_anchor(ElementID new_anchor) {
    anchor = new_anchor;
}

void Cursor::update_on_insert(const Document& doc, const InsertOperation& oper) {
    if (oper.get_parent() == get_anchor()) {
        set_anchor(oper.get_id());
    }
}

void Cursor::update_on_delete(const Document& doc, const RemoveOperation& oper) {
    if (oper.get_target() == get_anchor()) {
        set_anchor(doc.visible_predecessor(get_anchor()));
    }
}

std::pair<size_t, size_t> Cursor::get_line_column(const Document& doc) const {
    return doc.get_cursor_position(anchor);
}

void Cursor::update_desired_column(const Document& document) {
    auto [line, column] = get_line_column(document);
    desired_column = column;
}

void Cursor::update(const Document& doc, const Operation& oper) {
    std::visit([&](const auto& operation) { 
        using T = std::decay_t<decltype(operation)>;
        if constexpr (std::is_same_v<T, InsertOperation>) {
            update_on_insert(doc, operation);
        } else if constexpr (std::is_same_v<T, RemoveOperation>) {
            update_on_delete(doc, operation);
        }
    }, oper);
}

bool Cursor::has_desired_column() const {
    if (desired_column.has_value()) {
        return true;
    }
    return false;
}

void Cursor::set_desired_column(size_t column) {
    desired_column = column;
}

std::optional<size_t> Cursor::get_desired_column() const {
    return desired_column;
}

void Cursor::set_position(ElementID new_anchor, const Document& doc) {
    set_anchor(new_anchor);
    auto [line, column] = get_line_column(doc);
    desired_column = column;
}

Cursor::Cursor (ElementID p) : anchor(p) {}
