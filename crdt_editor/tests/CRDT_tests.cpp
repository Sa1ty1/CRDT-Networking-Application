#include <tests/CRDT_tests.hpp>

void insertion_in_middle_test() {
    Document document;
    Cursor cursor(ROOT_ID);
    Id_generator generator("local");
    InputHandler handler(document, cursor, generator);
    OperationLog log;
    EditorSession session(document, cursor, handler, log, generator);

    session.handle_editor_command(
        EditorCommand(InsertCharacter, 'a'));

    session.handle_editor_command(
        EditorCommand(InsertCharacter, 'b'));

    session.handle_editor_command(
        EditorCommand(InsertCharacter, 'c'));

    // a b c
    session.handle_editor_command(
        EditorCommand(MoveLeft));

    session.handle_editor_command(
        EditorCommand(MoveLeft));

    // a | b c
    session.handle_editor_command(
        EditorCommand(InsertCharacter, 'x'));

    // Should be: a x b c
    assert(document.render() == "axbc");
}