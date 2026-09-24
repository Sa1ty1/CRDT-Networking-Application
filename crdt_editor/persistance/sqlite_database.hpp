#pragma once
#include <iostream>
#include <string>
#include <sqlite3.h>
#include <vector>
#include <persistance/document_id.hpp>


class SQLiteDatabase {
public:
    explicit SQLiteDatabase(const std::string& filename);
    ~SQLiteDatabase();
    
    SQLiteDatabase(const SQLiteDatabase&) = delete;
    SQLiteDatabase& operator=(const SQLiteDatabase&) = delete;

    void insert_operation(const std::string& document_id, const std::string& operation_id, const std::string& operation_data);

    void insert_document(const DocumentID& document_id);

    void delete_document(const DocumentID& document_id);

    std::vector<DocumentID> get_document_ids() const;

    std::vector<std::string> get_operations(const DocumentID& document_id) const;

    void begin_transaction();
    void commit();
    void rollback();

private:
    void initialize_schema();

    void execute(const std::string& sql);

    sqlite3* db = nullptr;

};