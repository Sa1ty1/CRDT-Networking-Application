#include <editor/input_handler.hpp>

std::vector<Operation> InputHandler::insert_character(char c) {

    std::cout << "INSERT CURSOR: " << cursor.get_anchor().to_String() << '\n';

    cursor.clear_selection();
    ElementID parent = cursor.get_anchor();
    ElementID newID = gen.next();

    InsertOperation oper(parent, newID, c);
    cursor.set_anchor(newID);
    return {oper};
}

std::vector<Operation> InputHandler::move_left_impl() {
    std::cout << "starting move_left" << std::endl;
    cursor.set_anchor(doc.visible_predecessor(cursor.get_anchor()));

    auto [line, column] = cursor.get_line_column(doc);
    return {};
}

std::vector<Operation> InputHandler::move_right_impl() {
    std::cout << "starting move_right" << std::endl;
    auto next = doc.visible_successor(cursor.get_anchor());
    if (next) {
        cursor.set_anchor(*next);
    }
    
    auto [line, column] = cursor.get_line_column(doc);
    return {};
}

std::vector<Operation> InputHandler::backspace() {
    if (cursor.has_selection()) {
        return delete_selection();
    }

    std::cout << "starting backspace" << std::endl;
    RemoveOperation oper = RemoveOperation(cursor.get_anchor());
    cursor.set_anchor(doc.visible_predecessor(cursor.get_anchor()));
    std::vector<Operation> vec = {oper};
    return vec;
}

std::vector<Operation> InputHandler::delete_forward() { // need to implement this
    if (cursor.has_selection()) {
        return delete_selection();
    }

    std::cout << "starting delete_forward" << std::endl;
    auto next = doc.visible_successor(cursor.get_anchor());
    if (next) {
        RemoveOperation oper = RemoveOperation(*next);
        std::vector<Operation> vec = {oper};
        return vec;
    }
    return {};
}

std::vector<Operation> InputHandler::move_up_impl() {
    auto [line, column] = cursor.get_line_column(doc);

    if (line == 0) { // do we just send to ROOT_ID?
        return {};
    }

    if (!cursor.has_desired_column()) {
        cursor.set_desired_column(column);
    }

    size_t target_line = line - 1;

    auto length = doc.get_line_length(target_line);
    
    auto des_col_val = cursor.get_desired_column();

    size_t target_column = std::min(*des_col_val, *length);
    auto anchor = doc.get_anchor_at(target_line, target_column);

    if (anchor) {
        cursor.set_anchor(*anchor);
    }
    auto [line1, column1] = cursor.get_line_column(doc);

    return {};
}


std::vector<Operation> InputHandler::move_down_impl() {
    auto [line, column] = cursor.get_line_column(doc);

    size_t target_line = line + 1;

    auto length = doc.get_line_length(target_line);

    if (!length) {
        return {}; //dealing iwth last line
    }

    if (!cursor.has_desired_column()) {
        cursor.set_desired_column(column);
    }

    auto des_col_val = cursor.get_desired_column();

    size_t target_column = std::min(*des_col_val, *length);

    auto anchor = doc.get_anchor_at(target_line, target_column);

    if (anchor) {
        cursor.set_anchor(*anchor);
    }

    auto [line1, column1] = cursor.get_line_column(doc);
    return {};
}

std::vector<Operation> InputHandler::newline() {
    return insert_character('\n');
}

std::vector<Operation> InputHandler::move_left() {
    std::cout << "starting move left" << std::endl;
    cursor.clear_selection();
    return move_left_impl();
}

std::vector<Operation> InputHandler::move_right() {
    std::cout << "starting move right" << std::endl;
    cursor.clear_selection();
    return move_right_impl();
}

std::vector<Operation> InputHandler::move_up() {
    std::cout << "starting move up" << std::endl;
    cursor.clear_selection();
    return move_up_impl();
}

std::vector<Operation> InputHandler::move_down() {
    std::cout << "starting move down" << std::endl;
    cursor.clear_selection();
    return move_down_impl();
}

std::vector<Operation> InputHandler::shift_left() {
    if (!cursor.has_selection()) {
        cursor.start_selection();
    }
    return move_left_impl();
}

std::vector<Operation> InputHandler::shift_right() {
    if (!cursor.has_selection()) {
        cursor.start_selection();
    }
    return move_right_impl();
}

std::vector<Operation> InputHandler::shift_up() {
        if (!cursor.has_selection()) {
        cursor.start_selection();
    }
    return move_up_impl();
}

std::vector<Operation> InputHandler::shift_down() {
    if (!cursor.has_selection()) {
        cursor.start_selection();
    }
    return move_down_impl();
}

std::vector<Operation> InputHandler::delete_selection() {
    if (!cursor.has_selection()) {
        return {};
    }
    auto range = cursor.normalized_range(doc);
    if (!range) {
        return {};
    }
    std::vector<ElementID> selected = doc.get_visible_range(*range);
    if (selected.empty()) {
        cursor.clear_selection();
        return {};
    }
    ElementID new_cursor = doc.visible_predecessor(selected.front());
    std::vector<Operation> result;
    for (const auto& id : selected) {
        RemoveOperation oper = RemoveOperation(id);
        result.emplace_back(oper);
    }
    cursor.clear_selection();
    cursor.set_anchor(new_cursor);
    return result;
}

std::vector<Operation> InputHandler::process_command(EditorCommand command) {
    switch (command.get_type())
    {
    case InsertCharacter:
        return insert_character(command.get_character());
    case MoveLeft:
        return move_left();
    case MoveRight:
        return move_right();
    case Backspace:
        return backspace();
    case DeleteForward:
        return delete_forward();
    case Enter:
        return newline();
    case MoveUp:
        return move_up();
    case MoveDown:
        return move_down();
    case ShiftLeft:
        return shift_left();
    case ShiftRight:
        return shift_right();
    case ShiftUp:
        return shift_up();
    case ShiftDown:
        return shift_down();
    default:
        throw(std::invalid_argument("This is not a valid command")); //probably should build a to_string for command types
        std::vector<Operation> empty;
        return empty;
    }
}

InputHandler::InputHandler(Document& d, Cursor& c, Id_generator& g): doc(d), cursor(c), gen(g) {}
