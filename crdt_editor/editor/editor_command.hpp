#pragma once
#include <iostream>

enum CommandType {InsertCharacter, MoveLeft, MoveRight, MoveUp, MoveDown, Backspace, DeleteForward, Enter,
                    ShiftLeft, ShiftRight, ShiftUp, ShiftDown};

class EditorCommand {
public:
    CommandType get_type() const;
    char get_character() const;

    EditorCommand(CommandType command, char ch);

    EditorCommand(CommandType command);

private:
    char character;
    CommandType type;
};