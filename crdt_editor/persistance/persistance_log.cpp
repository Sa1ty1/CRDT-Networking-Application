#include <persistance/persistance_log.hpp>



PersistentOperationLog::PersistentOperationLog(SQLiteDatabase& database, DocumentID document_id) : database(database), document_id(std::move(document_id)) {
    
}

std::vector<Operation> PersistentOperationLog::load() const {
    const auto serialized_operations = database.get_operations(document_id);

    std::vector<Operation> operations;
    operations.reserve(serialized_operations.size());

    for (const auto& data : serialized_operations) {
        operations.emplace_back(operation_serializer::deserialize(data));
    }
    return operations;
}

void PersistentOperationLog::record(const Operation& operation) {
    const std::string operation_data = operation_serializer::serialize(operation);

    const std::string operation_id = hash(operation_data);
    
    database.insert_operation(document_id, operation_id, operation_data);
    
    // Insert into the database
    // prepare INSERT
    // bind document_id
    // bind operation_id
    // bind operation_data
    // execute
}