#include <UI/viewport.hpp>

size_t Viewport::get_first_line() const {
    return first_line;
}
size_t Viewport::get_first_column() const {
    return first_column;
}

void Viewport::set_first_line(size_t line) {
    first_line = line;
}

void Viewport::set_first_column(size_t column) {
    first_column = column;
}

void Viewport::scroll_lines(int amount) {
    if (amount < 0) {
        size_t distance = static_cast<size_t>(-amount);
        first_line = (distance > first_line) ? 0 : first_line - distance;
    } else {
        first_line += static_cast<size_t>(amount);
    }
}

void Viewport::scroll_columns(int amount) {
    if (amount < 0) {
        size_t distance = static_cast<size_t>(-amount);
        first_column = (distance > first_column) ? 0 : first_column - distance;
    } else {
        first_column += static_cast<size_t>(amount);
    }
}

void Viewport::clamp(size_t document_lines, size_t visible_lines, size_t document_columns, size_t visible_columns) {
    if (visible_lines == 0 || document_lines <= visible_lines) {
        first_line = 0;
        return;
    } else {
        first_line = std::min(first_line, document_lines - visible_lines);
    }

    if (visible_columns == 0 || document_columns <= visible_columns) {
        first_column = 0;
        return;
    } else {
        first_column = std::min(first_column, document_columns - visible_columns);

    }
}