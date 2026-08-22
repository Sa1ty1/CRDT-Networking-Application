#pragma once

class Character {
public:
    char get_char() const;
    bool get_deleted() const;
    void set_deleted(bool is_del);

    Character(char c, bool is_deleted);

private:
    char actual_char;
    bool deleted;
};