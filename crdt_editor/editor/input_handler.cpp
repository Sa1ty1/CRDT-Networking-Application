#include <editor/input_handler.hpp>

std::vector<Operation> InputHandler::insert_character(char c) {

    std::cout << "INSERT CURSOR: " << cursor.get_anchor().to_String() << '\n';
    std::cout << "INSERT CURSOR POSITION: ";
    auto [line, column] = cursor.get_line_column(doc);
    std::cout << line << ", " << column << '\n';

    ElementID parent = cursor.get_anchor();
    ElementID newID = gen.next();
    InsertOperation oper = InsertOperation(parent, newID, c);
    cursor.set_anchor(newID);
    return std::vector<Operation>{oper};
}

std::vector<Operation> InputHandler::move_left() {
    std::cout << "starting move_left" << std::endl;
    // if (cursor.get_anchor() == ROOT_ID) { // see if this needs to come back
    //     return {};
    // }
    cursor.set_anchor(doc.visible_predecessor(cursor.get_anchor()));
    //cursor.update_desired_column(doc); // if this breaks its probably about ROOT_ID problems

    auto [line, column] = cursor.get_line_column(doc);
    std::cout << "After Move Cursor in move left: " << line << ", " << column << '\n';
    return {};
}

std::vector<Operation> InputHandler::move_right() {
    std::cout << "starting move_right" << std::endl;
    auto next = doc.visible_successor(cursor.get_anchor());
    if (next) {
        cursor.set_anchor(*next);
        //cursor.update_desired_column(doc);
    }
    
    auto [line, column] = cursor.get_line_column(doc);
    std::cout << "After Move Cursor in move right: " << line << ", " << column << '\n';

    return {};
}

std::vector<Operation> InputHandler::backspace() {
    std::cout << "starting backspace" << std::endl;
    RemoveOperation oper = RemoveOperation(cursor.get_anchor());
    cursor.set_anchor(doc.visible_predecessor(cursor.get_anchor()));
    std::vector<Operation> vec = {oper};
    return vec;
}

std::vector<Operation> InputHandler::delete_forward() { // need to implement this
    std::cout << "starting delete_forward" << std::endl;
    auto next = doc.visible_successor(cursor.get_anchor());
    if (next) {
        RemoveOperation oper = RemoveOperation(*next);
        std::vector<Operation> vec = {oper};
        return vec;
    }
    return {};
}

std::vector<Operation> InputHandler::move_up() {
    auto [line, column] = cursor.get_line_column(doc);

    if (line == 0) { // do we just send to ROOT_ID?
        return {};
    }

    if (!cursor.has_desired_column()) {
        cursor.set_desired_column(column);
    }
    std::cout << "desired column worked: " << cursor.get_desired_column().value() << std::endl;

    size_t target_line = line - 1;

    auto length = doc.get_line_length(target_line);
    if (length.has_value()) {
        std::cout << "get line length worked: " << length.value() << std::endl;
    } else {
        std::cout << "length has no value" << std::endl;
    }
    
    auto des_col_val = cursor.get_desired_column();

    size_t target_column = std::min(*des_col_val, *length);
    std::cout << "target_column worked: " << target_column << std::endl;
    auto anchor = doc.get_anchor_at(target_line, target_column);
    std::cout << "get anchor at worked" << std::endl;

    if (anchor) {
        cursor.set_anchor(*anchor);
    }
    auto [line1, column1] = cursor.get_line_column(doc);
    std::cout << "After Move Cursor in move up: " << line1 << ", " << column1 << '\n';

    return {};
}


std::vector<Operation> InputHandler::move_down() {
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
    std::cout << "After Move Cursor in move down: " << line1 << ", " << column1 << '\n';
    return {};
}

std::vector<Operation> InputHandler::newline() {
    return insert_character('\n');
}

std::vector<Operation> InputHandler::process_command(EditorCommand command) {
    std::cout << "Command = " << command.get_type() << '\n';
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
    default:
        throw(std::invalid_argument("This is not a valid command")); //probably should build a to_string for command types
        std::vector<Operation> empty;
        return empty;
    }
}

InputHandler::InputHandler(Document& d, Cursor& c, Id_generator& g): doc(d), cursor(c), gen(g) {}
