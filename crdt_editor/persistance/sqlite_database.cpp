#include <persistance/sqlite_database.hpp>

SQLiteDatabase::SQLiteDatabase(const std::string& filename) {
    int rc = sqlite3_open(filename.c_str(), &db);

    if (rc != SQLITE_OK) {
        std::string error = sqlite3_errmsg(db);
        sqlite3_close(db);
        db = nullptr;
        throw std::runtime_error("Failed to open database: " + error);
    }

    execute("PRAGMA foreign_keys = ON;");
    initialize_schema();
}

SQLiteDatabase::~SQLiteDatabase() {
    if (db != nullptr) {
        sqlite3_close(db);
    }
}

void SQLiteDatabase::execute(const std::string& sql) {
    char* error_message = nullptr;

    int rc = sqlite3_exec(
        db,
        sql.c_str(),
        nullptr,
        nullptr,
        &error_message
    );
    if (rc != SQLITE_OK) {
        std::string error = error_message ? error_message : "Unknown SQLite error";
        sqlite3_free(error_message);
        throw std::runtime_error("SQLite error: " + error);
    }
}

void SQLiteDatabase::insert_operation(const std::string& document_id, const std::string& operation_id, const std::string& operation_data) {
    const char* sql = 
        "INSERT OR IGNORE INTO operations "
        "(document_id, operation_id, operation_data)"
        "VALUES (?, ?, ?);";
    
    sqlite3_stmt* statement = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, & statement, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare operation insert: " + std::string(sqlite3_errmsg(db)));
    }

    rc = sqlite3_bind_text(statement, 1, document_id.c_str(), -1, SQLITE_TRANSIENT);
    
    if (rc != SQLITE_OK) {
        sqlite3_finalize(statement);
        throw std::runtime_error("Failed to bind document ID");
    }

    rc = sqlite3_bind_text(statement, 2, operation_id.c_str(), -1, SQLITE_TRANSIENT);
    
    if (rc != SQLITE_OK) {
        sqlite3_finalize(statement);
        throw std::runtime_error("Failed to bind operation ID");
    }

    rc = sqlite3_bind_text(statement, 3, operation_data.c_str(), -1, SQLITE_TRANSIENT);
    
    if (rc != SQLITE_OK) {
        sqlite3_finalize(statement);
        throw std::runtime_error("Failed to bind operation data");
    }
    
    rc = sqlite3_step(statement);

    if (rc != SQLITE_DONE) {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(statement);
        throw std::runtime_error("Failed to insert operation: " + error);
    }
    
    sqlite3_finalize(statement);
}

std::vector<DocumentID> SQLiteDatabase::get_document_ids() const {
    const char* sql = "SELECT document_id FROM documents;";

    sqlite3_stmt* statement = nullptr;

    int rc = sqlite3_prepare_v2(
        db, sql, -1, &statement, nullptr
    );

    if (rc != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare document query: " + std::string(sqlite3_errmsg(db)));
    }

    std::vector<DocumentID> document_ids;

    while ((rc = sqlite3_step(statement)) == SQLITE_ROW) {
        const unsigned char* value = sqlite3_column_text(statement, 0);

        if (value == nullptr) {
            sqlite3_finalize(statement);
            throw std::runtime_error("Document ID was NULL");
        }

        document_ids.emplace_back(reinterpret_cast<const char*>(value));
    }

    if (rc != SQLITE_DONE) {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(statement);
        throw std::runtime_error("Failed to read documents: " + error);
    }

    sqlite3_finalize(statement);

    return document_ids;
}

void SQLiteDatabase::delete_document(const DocumentID& document_id) {

    try {
        begin_transaction();

        const char* delete_operations = "DELETE FROM operations WHERE document_id = ?;";
        sqlite3_stmt* statement = nullptr;

        int rc = sqlite3_prepare_v2(db, delete_operations, -1, &statement, nullptr);

        if (rc != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare operation deletion: " + std::string(sqlite3_errmsg(db)));
        }
        rc = sqlite3_bind_text(statement, 1, document_id.c_str(), -1, SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) {
            sqlite3_finalize(statement);
            throw std::runtime_error("Failed to bind document ID");
        }

        rc = sqlite3_step(statement);

        if (rc != SQLITE_DONE) {
            std::string error = sqlite3_errmsg(db);
            sqlite3_finalize(statement);

            throw std::runtime_error("Failed to delete operations: " + error);
        }
        sqlite3_finalize(statement);

        const char* delete_document = "DELETE FROM documents WHERE document_id = ?;";

        rc = sqlite3_prepare_v2(db, delete_document, -1, &statement, nullptr);
        if (rc != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare document deletion: " + std::string(sqlite3_errmsg(db)));
        }

        rc = sqlite3_bind_text(statement, 1, document_id.c_str(), -1, SQLITE_TRANSIENT);

        if (rc != SQLITE_OK) {
            sqlite3_finalize(statement);
            throw std::runtime_error("Failed to bind document ID");
        }

        rc = sqlite3_step(statement);

        if (rc != SQLITE_DONE) {
            std::string error = sqlite3_errmsg(db);
            sqlite3_finalize(statement);
            throw std::runtime_error("Failed to delete document: " + error);
        }

        sqlite3_finalize(statement);

        commit();
    } catch (...) { // ... catches any exception
        rollback();
        throw;
    }
}

void SQLiteDatabase::insert_document(const DocumentID& document_id) {
    const char* sql = "INSERT INTO documents (document_id) VALUES (?);";

    sqlite3_stmt* statement = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &statement, nullptr);

    if (rc != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare document insert: " + std::string(sqlite3_errmsg(db)));
    }
    
    rc = sqlite3_bind_text(statement, 1, document_id.c_str(), -1, SQLITE_TRANSIENT);

    if (rc != SQLITE_OK) {
        sqlite3_finalize(statement);
        throw std::runtime_error("Failed to bind document ID");
    }

    rc = sqlite3_step(statement);

    if (rc != SQLITE_DONE) {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(statement);

        throw std::runtime_error("Failed to insert document: " + error);
    }

    sqlite3_finalize(statement);
}

void SQLiteDatabase::begin_transaction() {
    execute("BEGIN TRANSACTION;");
}

void SQLiteDatabase::commit() {
    execute("COMMIT;");
}

void SQLiteDatabase::rollback() {
    execute("ROLLBACK;");
}

std::vector<std::string> SQLiteDatabase::get_operations(const DocumentID& document_id) const {
    const char* sql = "SELECT operation_data FROM operations WHERE document_id = ?;";

    sqlite3_stmt* statement = nullptr;
    int rc = sqlite3_prepare_v2(db, sql, -1, &statement, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare operation query: " + std::string(sqlite3_errmsg(db)));
    }

    rc = sqlite3_bind_text(statement, 1, document_id.c_str(), -1, SQLITE_TRANSIENT);

    if (rc != SQLITE_OK) {
        sqlite3_finalize(statement);
        throw std::runtime_error("Failed to bind document ID");
    }

    std::vector<std::string> operations;
    while ((rc = sqlite3_step(statement)) == SQLITE_ROW) {
        const unsigned char* value = sqlite3_column_text(statement, 0);
        if (value == nullptr) {
            sqlite3_finalize(statement);
            throw std::runtime_error("Operation data was NULL");
        }
        operations.emplace_back(reinterpret_cast<const char*>(value));
    }
    if (rc != SQLITE_DONE) {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(statement);
        throw std::runtime_error("Failed to read operations: " + error);
    }
    sqlite3_finalize(statement);

    return operations;
}


void SQLiteDatabase::initialize_schema() {
    execute(
        "CREATE TABLE IF NOT EXISTS documents ("
        "document_id TEXT PRIMARY KEY);"
    );
    execute(
        "CREATE TABLE IF NOT EXISTS operations ("
        "document_id TEXT NOT NULL,"
        "operation_id TEXT NOT NULL,"
        "operation_data TEXT NOT NULL,"
        "PRIMARY KEY (document_id, operation_id),"
        "FOREIGN KEY (document_id) REFERENCES documents(document_id)"
        ");"
    );
}