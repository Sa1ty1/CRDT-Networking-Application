// what do we want it to do
// when an operation is applied to the document, we keep a copy of the operation in the log
// can be used to retrace the operations and rebuild a document
#pragma once
#include <iostream>
#include <vector>
#include <unordered_set>
#include <string>
#include <src/operation.hpp>
#include <set>

class OperationLog {
public:

    std::vector<Operation> get_log() const;
    void record(const Operation& oper);
    int size() const;
    void clear();
    std::set<std::string> to_set();
    std::vector<std::string> to_vector_of_serialized_operations();
    std::string to_String();
    OperationLog();
    OperationLog(std::vector<Operation> log);

private:
    std::vector<Operation> operation_history;
    std::unordered_set<std::string> recorded_operations;
};