#include <persistance/persistent_document.hpp>

PersistentDocument::PersistentDocument(SQLiteDatabase& database, DocumentID document_id): persistent_log(database, std::move(document_id)) {}

Document& PersistentDocument::get_document() {
    return document;
}


const Document& PersistentDocument::get_document() const {
    return document;
}

void PersistentDocument::apply_operation(const Operation& operation) {
    /* 
    Persistence happens before modifying the in-memory document.
    If persistent_log.record() fails, throws and Document is unchanged. 
    Prevents the in-memory state advancing beyond durable state. 
    */
   persistent_log.record(operation);
   document.apply(operation);
}

std::vector<Operation> PersistentDocument::get_history() const {
    return persistent_log.load();
}

void PersistentDocument::load() {
    /*
    Reconstruct the current document state by 
    replaying the persisted operation history. 
    */
   const std::vector<Operation> history = persistent_log.load();
   for (const auto& operation : history) {
    document.apply(operation);
   }
}




/*
SCHEMA:

CREATE TABLE documents (
    document_id TEXT PRIMARY KEY
);

CREATE TABLE operations (
    document_id TEXT NOT NULL,
    operation_id TEXT NOT NULL,
    operation_data TEXT NOT NULL,

    PRIMARY KEY (document_id, operation_id),

    FOREIGN KEY (document_id)
        REFERENCES documents(document_id)
);
*/