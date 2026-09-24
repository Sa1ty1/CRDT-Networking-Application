#include <persistance/document_store.hpp>


DocumentStore::DocumentStore() : database("documents.db") {
    load();
}

bool DocumentStore::exists(const DocumentID& id) const {
    return documents.contains(id);
}

PersistentDocument& DocumentStore::get_document(const DocumentID& id) {
    auto it = documents.find(id);

    if (it == documents.end()) {
        throw std::runtime_error("Document does not exist: " + id);
    }
    return it->second;
}

PersistentDocument& DocumentStore::create_document(const DocumentID& id) {
    if (exists(id)) {
        throw std::runtime_error("Document already exists: " + id);
    }

    database.insert_document(id);

    auto [it, inserted] = documents.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(database, id));
    if (!inserted) {
        throw std::runtime_error("Failed to create document: " + id);
    }
    return it->second;
}

void DocumentStore::remove_document(const DocumentID& id) {
    auto it = documents.find(id);
    if (it == documents.end()) {
        throw std::runtime_error("Document does not exist: " + id);
    }
    database.delete_document(id);
    documents.erase(it);
}

std::vector<DocumentID> DocumentStore::list_documents() const {
    std::vector<DocumentID> ids;
    ids.reserve(documents.size());
    for (const auto& [id, document] : documents) {
        ids.push_back(id);
    }
    return ids;
}

void DocumentStore::load() {
    const auto document_ids = database.get_document_ids();
    for (const auto& id: document_ids) {
        auto [it, inserted] = documents.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(id),
            std::forward_as_tuple(database, id)
        );

        if (!inserted) {
            throw std::runtime_error("Document already loaded: " + id);
        }
        it->second.load();
    }
}