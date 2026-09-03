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
    std::cout << "LOCAL CURSOR DELETE UPDATE\n";

    if (oper.get_target() != get_anchor()) {
        return;
    }

    ElementID predecessor = doc.visible_predecessor(oper.get_target());
    set_anchor(predecessor);
}

void Cursor::update_on_remote(const Document& doc, const Operation& oper) {
    // Remote operations: local cursor must still be fixed if its anchor was deleted
    std::visit([&] (const auto& operation) {
        using T = std::decay_t<decltype(operation)>;
        if constexpr (std::is_same_v<T, RemoveOperation>) {
            update_on_delete(doc, operation);
        }
    }, oper);
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

bool Cursor::has_selection() const {
    return selection_anchor.has_value() && selection_anchor.value() != anchor;
}

const ElementID& Cursor::get_selection_anchor() const {
    if (has_selection()) {
        return selection_anchor.value();
    }
    throw std::runtime_error("tried to get a selection anchor without a value");
}

void Cursor::start_selection() {
    selection_anchor = anchor;
}

void Cursor::clear_selection() {
    if (selection_anchor.has_value()) {
        selection_anchor.reset();
    }
}

std::optional<DocumentRange> Cursor::normalized_range(const Document& doc) const {
    if (!has_selection()) {
        return std::nullopt;
    }

    const ElementID& anchor = get_selection_anchor();
    const ElementID& cursor = get_anchor();

    if (anchor == cursor) {
        return std::nullopt;
    }
    if (anchor == ROOT_ID) {
        return DocumentRange{anchor, cursor};
    }
    if (cursor == ROOT_ID) {
        return DocumentRange{cursor, anchor};
    }

    std::optional<DocumentRange> result;

    doc.visit_visible(ROOT_ID, [&] (const ElementID& id) {
        if (result.has_value()) {
            return;
        }
        if (id == anchor) {
            result = DocumentRange{anchor, cursor};
        } else if (id == cursor) {
            result = DocumentRange{cursor, anchor};
        }
    });
    return result;
}


Cursor::Cursor (ElementID p) : anchor(p) {}
