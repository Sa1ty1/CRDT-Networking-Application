#pragma once
#include <persistance/persistance_log.hpp>
#include <src/document.hpp>

class PersistentDocument {
public:
    PersistentDocument(SQLiteDatabase& database, DocumentID document_id);

    Document& get_document();
    const Document& get_document() const;
    void apply_operation(const Operation& operation);
    std::vector<Operation> get_history() const;

    void load();

private:
    Document document;
    PersistentOperationLog persistent_log;
};