
#include <src/operation_log.hpp>


std::vector<Operation> OperationLog::get_log() const {return operation_history;}

void OperationLog::record(const Operation& oper) {
    std::string serialized = operation_serializer::serialize(oper);
    if (!recorded_operations.insert(serialized).second) {
        return; // already recorded
    }
    operation_history.push_back(oper);
}

int OperationLog::size() const {
    return operation_history.size();
}

void OperationLog::clear() {
    operation_history.clear();
}

std::set<std::string> OperationLog::to_set() {
    std::vector<std::string> res = to_vector_of_serialized_operations();
    return std::set<std::string>(res.begin(), res.end());
}

std::vector<std::string> OperationLog::to_vector_of_serialized_operations() {
    std::vector<std::string> output;
    for (int i = 0; i < operation_history.size(); i++) {
        output.emplace_back(operation_serializer::serialize(operation_history.at(i)));
    }
    return output;
}
    
std::string OperationLog::to_String() {
    std::string output;
    for (int i = 0; i < operation_history.size(); i++) {
        output.append(operation_serializer::serialize(operation_history.at(i)));
        output.append(", ");
    }
    return output;
}

OperationLog::OperationLog() = default;

OperationLog::OperationLog(std::vector<Operation> log) : operation_history(log) {}
