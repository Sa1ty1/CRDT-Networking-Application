#pragma once
#include <algorithm>
#include <cstddef>

class Viewport {
public:
    Viewport() = default;
    size_t get_first_line() const;
    size_t get_first_column() const;

    void set_first_line(size_t line);
    void set_first_column(size_t column);

    void scroll_lines(int amount);
    void scroll_columns(int amount);
    void clamp(size_t document_lines, size_t visible_lines, size_t document_columns, size_t visible_columns);

private:
    size_t first_line = 0;
    size_t first_column = 0;
};