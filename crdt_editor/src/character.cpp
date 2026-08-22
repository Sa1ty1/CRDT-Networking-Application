#include <src/character.hpp>

char Character::get_char() const {return actual_char;}
bool Character::get_deleted() const {return deleted;}
void Character::set_deleted(bool is_del) {deleted = is_del;}

Character::Character(char c, bool is_deleted): actual_char(c), deleted(is_deleted) {}
