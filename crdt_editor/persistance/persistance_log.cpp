#include <persistance/persistance_log.hpp>



PersistentOperationLog::PersistentOperationLog(const std::string& filename) : filename(filename) {
    std::ofstream file(filename, std::ios::app);

    if (!file) {
        throw std::runtime_error("Failed to open persistant operation log");
    }
}

std::vector<Operation> PersistentOperationLog::load() const {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Failed to open persistent operation log");
    }

    std::vector<Operation> operations;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        operations.push_back(operation_serializer::deserialize(line));
    }
    return operations;
}

void PersistentOperationLog::record(const Operation& operation) {
    std::ofstream file(filename, std::ios::app);

    if (!file) {
        throw std::runtime_error("Failed to open persistent operation log");
    }

    file << operation_serializer::serialize(operation) << '\n';
}