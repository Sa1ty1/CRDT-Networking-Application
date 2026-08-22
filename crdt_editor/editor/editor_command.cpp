#include <editor/editor_command.hpp>


CommandType EditorCommand::get_type() const {return type;}

char EditorCommand::get_character() const {
    if (!character) {
        throw std::runtime_error("No character in this command.");
    }
    return character;
}

EditorCommand::EditorCommand(CommandType command, char ch) {
    type = command;
    character = ch;
}

EditorCommand::EditorCommand(CommandType command) {
    type = command;
}
