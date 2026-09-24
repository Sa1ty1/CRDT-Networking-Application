#pragma once
#include <iostream>
#include <unordered_map>
#include <filesystem>
#include <src/document.hpp>
#include <persistance/persistent_document.hpp>
#include <persistance/document_id.hpp>
#include <persistance/sqlite_database.hpp>

class DocumentStore {
public:
    std::vector<DocumentID> list_documents() const;
    bool exists(const DocumentID& id) const;
    PersistentDocument& get_document(const DocumentID& id);
    PersistentDocument& create_document(const DocumentID& id);
    void remove_document(const DocumentID& id);
    void load();

    DocumentStore();

private:
    SQLiteDatabase database;
    std::unordered_map<DocumentID, PersistentDocument>  documents;
};