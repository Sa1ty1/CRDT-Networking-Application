#include <persistance/document_store.hpp>


DocumentStore::DocumentStore() {
    std::filesystem::create_directories("data");
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
    std::string filename = "data/" + id + ".log";
    auto [it, inserted] = documents.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(filename));
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
    //TODO
}