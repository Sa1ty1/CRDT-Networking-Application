#include <tests/UI_tests.hpp>

ElementID find_character(const Document& document, char target)
{
    std::optional<ElementID> result;

    document.visit_visible(ROOT_ID, [&](const ElementID& id) {
        if (result) {
            return;
        }

        if (document.get_character(id) == target) {
            result = id;
        }
    });

    if (!result) {
        throw std::runtime_error("Character not found");
    }

    return *result;
}

void row_col_test() {

    std::cout << "\n--- test_row_col_function ---\n";
    Document documentA;
    Cursor cursorA(ROOT_ID);
    Id_generator generatorA("client");
    InputHandler handlerA(documentA, cursorA, generatorA);
    OperationLog logA;
    EditorSession sessionA(documentA, cursorA, handlerA, logA, generatorA);

    sessionA.handle_editor_command(EditorCommand(InsertCharacter, 'H'));
    sessionA.handle_editor_command(EditorCommand(InsertCharacter, 'E'));
    sessionA.handle_editor_command(EditorCommand(InsertCharacter, 'L'));
    sessionA.handle_editor_command(EditorCommand(InsertCharacter, 'L'));
    sessionA.handle_editor_command(EditorCommand(InsertCharacter, 'O'));
    sessionA.handle_editor_command(EditorCommand(InsertCharacter, '\n'));
    sessionA.handle_editor_command(EditorCommand(InsertCharacter, 'W'));
    sessionA.handle_editor_command(EditorCommand(InsertCharacter, 'O'));
    sessionA.handle_editor_command(EditorCommand(InsertCharacter, 'R'));
    sessionA.handle_editor_command(EditorCommand(InsertCharacter, 'L'));
    sessionA.handle_editor_command(EditorCommand(InsertCharacter, 'D'));
    sessionA.handle_editor_command(EditorCommand(InsertCharacter, '!'));

    assert(documentA.get_line_column(find_character(documentA, 'H')) == std::make_pair(0ULL, 0ULL));

    assert(documentA.get_line_column(find_character(documentA, 'E')) == std::make_pair(0ULL, 1ULL));

    assert(documentA.get_line_column(find_character(documentA, 'O')) == std::make_pair(0ULL, 4ULL));

    assert(documentA.get_line_column(find_character(documentA, 'W')) == std::make_pair(1ULL, 0ULL));

    assert(documentA.get_elements().at(documentA.get_element_at(0, 0).value()).get_char() == 'H');
    assert(documentA.get_elements().at(documentA.get_element_at(0, 4).value()).get_char() == 'O');
    assert(documentA.get_elements().at(documentA.get_element_at(1, 0).value()).get_char() == 'W');
    
    std::cout << "PASSED" << std::endl;
}