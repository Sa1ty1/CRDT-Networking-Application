#include <tests/persistance_tests.hpp>
#include <persistance/persistance_log.hpp>

void persistance_basic_test() {
    std::cout << "--- test_basic_persistance ---" << std::endl;

    std::filesystem::remove("test_operations.log");

    PersistentOperationLog log("test_operations.log");

    InsertOperation op1 = InsertOperation(ROOT_ID, ElementID(1, "s"), 'A');
    InsertOperation op2 = InsertOperation(ElementID(1, "s"), ElementID(3, "p"), 'B');


    log.record(op1);
    log.record(op2);

    auto operations = log.load();

    assert(operations.size() == 2);

    const auto& op1_loaded = std::get<InsertOperation>(operations[0]);
    const auto& op2_loaded = std::get<InsertOperation>(operations[1]);

    assert(op1_loaded.get_char() == op1.get_char());
    assert(op2_loaded.get_char() == op2.get_char());
    std::cout << "PASSED" << std::endl;
}

// Still need to check these

// Server loads PersistentOperationLog into OperationLog on startup.
// Test server restart → new client receives old document.
// Test empty persistence file / first-ever startup.