#pragma once
#include <fstream>
#include <stdexcept>
#include <vector>
#include <string>
#include <persistance/sqlite_database.hpp>
#include <persistance/document_id.hpp>
#include <src/operation.hpp>
#include <util/hash.hpp>

class PersistentOperationLog {
public:
    explicit PersistentOperationLog(SQLiteDatabase& database, DocumentID document_id);

    void record(const Operation& operation);

    std::vector<Operation> load() const;

private:
    SQLiteDatabase& database;
    DocumentID document_id;
};