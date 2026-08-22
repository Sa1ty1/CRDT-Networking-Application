#pragma once
#include <fstream>
#include <stdexcept>
#include <vector>
#include <string>
#include <src/operation.hpp>

class PersistentOperationLog {
public:
    explicit PersistentOperationLog(const std::string& filename);

    void record(const Operation& operation);

    std::vector<Operation> load() const;

private:
    std::string filename;
};