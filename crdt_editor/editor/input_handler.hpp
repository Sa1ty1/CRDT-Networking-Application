#pragma once
#include <crdt/id_generator.hpp>
#include <editor/cursor.hpp>
#include <src/document.hpp>
#include <editor/editor_command.hpp>
#include <src/operation_log.hpp>

class InputHandler {
public:

    std::vector<Operation> insert_character(char c);

    std::vector<Operation> move_left();

    std::vector<Operation> move_right();

    std::vector<Operation> backspace();

    std::vector<Operation> delete_forward();

    std::vector<Operation> newline();

    std::vector<Operation> move_up();

    std::vector<Operation> move_down();

    std::vector<Operation> process_command(EditorCommand command);

    InputHandler(Document& d, Cursor& c, Id_generator& g);

private:
    Document& doc;
    Cursor& cursor;
    Id_generator& gen;
};