#include <network/server.hpp>
#include <network/client_connection.hpp>

// create listening socket -> bind to port -> start listening -> begin accepting clients
Server::Server(boost::asio::io_context& io, unsigned short port): acceptor(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {
    std::cout << "Server listening on port " << port << '\n';
    document_store.load();
    //load_persistent_state();
    accept_client();
}

void Server::receive_message(std::shared_ptr<ClientConnection> sender, std::string serialized_message) {
    try {
        Message message = Message::deserialize(serialized_message);
        switch(message.get_type()) {
            case MessageType::HELLO: {
                if (sender->is_registered()) {
                    std::cerr << "Client sent HELLO after registration" << std::endl;
                    sender->disconnect();
                    return;
                }
                std::string id = message.get_sender();
                std::cout << "Received HELLO from " << id << std::endl;

                try {
                    register_client(sender, id);
                } catch (const std::exception& e) {
                    std::cerr << "Registration failed: " << e.what() << std::endl;
                    sender->disconnect();
                }
                return;
            }

            case MessageType::OPERATION: {
                if (!sender->is_live()) {
                    std::cerr << "Recieved OPERATION from non-live client" << std::endl;
                    sender->disconnect();
                    return;
                }
                if (!sender->has_current_document()) {
                    std::cerr << "Live client has no current document" << std::endl;
                    sender->disconnect();
                    return;
                }
                const auto& op = std::get<Operation>(message.get_payload());
                const DocumentID& document_id = sender->get_current_document();

                PersistentDocument& document = document_store.get_document(document_id);
                document.apply_operation(op);
                // oplog.record(op);
                // persistant_log.record(op);
                route_message(sender, message);
                return;
            }

            case MessageType::SYNC_COMPLETE: {
                if (!sender->is_syncing()) {
                    std::cerr << "Received SYNC_COMPLETE from client that isn't syncing" << std::endl;
                    sender->disconnect();
                    return;
                }
                finish_sync(sender);
                return;
            }

            case MessageType::CURSOR_UPDATE: {
                if (!sender->is_live()) {
                    std::cerr << "Recieved CURSOR_UPDATE from non-live client" << std::endl;
                    sender->disconnect();
                    return;
                }
                route_cursor_update(sender, message);
                return;
            }

            case MessageType::OPEN_DOCUMENT: {
                if (!sender->is_registered()) {
                    std::cerr << "Received OPEN_DOCUMENT from unregistered client" << std::endl;
                    sender->disconnect();
                    return;
                }
                if (sender->has_current_document()) {
                    std::cerr << "Client already has a document open. This should be fixed in a later update." << std::endl;
                    sender->disconnect();
                    return;
                }
                const auto& request = std::get<OpenDocument>(message.get_payload());

                try {
                    open_document(sender, request.document_id);
                } catch (const std::exception& e) {
                    std::cerr << "Failed to open document: " << e.what() << std::endl;
                    sender->disconnect();
                }
                return;
            }

            default:
                std::cerr << "Unexpected message type" << std::endl;
                sender->disconnect();
                return;
        }
    } catch (const std::exception& e) {
        std::cerr << "Invalid message: " << e.what() << std::endl;
        sender->disconnect();
    }
}

void Server::finish_sync(std::shared_ptr<ClientConnection> client) {

    if (!client->is_syncing()) {
        return;
    }

    if (!client->has_current_document()) {
        throw std::runtime_error("Cannot finish sync: client has no current document.");
    }

    const std::string& client_id = client->get_client_id();

    auto it = pending_sync_operations.find(client_id);

    if (it == pending_sync_operations.end()) {
        return;
    }

    std::cout << "Finishing sync for " << client_id << std::endl;

    // send everything that happened while syncing
    for (const auto& operation: it->second) {
        Message message(MessageType::OPERATION, "server", operation);
        client->send(message.serialize());
    }

    pending_sync_operations.erase(it);

    client->mark_live();

    send_presence(client);

    std::cout << "Client " << client_id << " is now live" << std::endl;
}

void Server::unregister_client(std::shared_ptr<ClientConnection> connection) {

    const std::string& client_id = connection->get_client_id();
    auto it = clients.find(client_id);

    if (it == clients.end()) {
        return;
    }
    if (it->second != connection) {
        return;
    }

    std::cout << "Removing client: " << client_id << "\n";
    clients.erase(it);
    pending_sync_operations.erase(client_id);
    presence.erase(client_id);
}

void Server::send_history(std::shared_ptr<ClientConnection> connection) {
    if (!connection->has_current_document()) {
        throw std::runtime_error("Cannot send history: client has no current document.");
    }
    const DocumentID& document_id = connection->get_current_document();
    PersistentDocument& document = document_store.get_document(document_id);
    Message response(MessageType::SYNC_RESPONSE, "server", document.get_history());
    connection->send(response.serialize());
}

void Server::route_cursor_update(const std::shared_ptr<ClientConnection>& sender, const Message& message) {

    if (!sender->has_current_document()) {
        throw std::runtime_error("Cannot route message: sender has no current document.");
    }

    const auto& update = std::get<CursorUpdate>(message.get_payload());
    presence.at(sender->get_client_id()).cursor = update.position;

    std::string serialized = message.serialize();

    for (const auto& [client_id, client] : clients) {
        if (client == sender) {
            continue;
        }
        if (!client->has_current_document()) {
            continue;
        }
        if (client->get_current_document() != sender->get_current_document()) {
            continue;
        }

        if (client->is_live()) {
            std::cout << "Routing cursor to " << client_id << std::endl;
            client->send(serialized);
        }
    }
}

void Server::send_presence(const std::shared_ptr<ClientConnection>& client) {

    if (!client->has_current_document()) {
        throw std::runtime_error("Cannot send presence: client has no current document.");
    }

    const DocumentID& document_id = client->get_current_document();

    for (const auto& [client_id, client_presence] : presence) {

        if (client_id == client->get_client_id()) {
            continue; // dont send to self
        }
        if (!clients.contains(client_id)) {
            continue; // don't send to not connected clients
        }

        auto other = clients.at(client_id);

        if (!other->is_live()) {
            continue; // don't send to not live clients
        }
        if (!other->has_current_document()) {
            continue;
        }
        if (other->get_current_document() != document_id) {
            continue;
        }

        Message message(MessageType::CURSOR_UPDATE, client_id, CursorUpdate{client_presence.cursor});
        client->send(message.serialize());
    }
}


// begin asnychronously waiting for the next incoming TCP connection
void Server::accept_client() { // Pattern: Accept one client -> start handling that client -> immediately begin accepting the next client
    auto connection = std::make_shared<ClientConnection>(acceptor.get_executor(), *this);

    acceptor.async_accept(connection->socket(), [this, connection] (const boost::system::error_code& error) {
        if (error) {
            std::cerr << "Accept error: " << error.message() << std::endl;
        } else {
            std::cout << "Client Accepted" << std::endl;
            connection->start();
        }
        accept_client();
    });
}

void Server::register_client(std::shared_ptr<ClientConnection> connection, std::string client_id) {
    if (client_id.empty()) {
        throw std::runtime_error("Client ID cannot be empty");
    }
    if (clients.contains(client_id)) {
        throw std::runtime_error("Client ID already connected");
    }
    std::cout << "Registering [" << client_id << "]\n";

    clients.emplace(client_id, connection); // probably want some try catch or other failsafe stuff
    pending_sync_operations.emplace(client_id, std::vector<Operation>{});
    presence.emplace(client_id, ClientPresence{ElementID(0, "__ROOT__")}); //Default sets it at ROOT

    connection->set_client_id(client_id);
    connection->mark_registered();
    Message ack(MessageType::HELLO_ACK, "server", std::monostate{});
    connection->send(ack.serialize());
}

void Server::route_message(const std::shared_ptr<ClientConnection>& sender, const Message& message) {
    if (!sender->has_current_document()) {
        throw std::runtime_error("Cannot route message: sender has no current document.");
    }
    
    std::string serialized = message.serialize();
    const auto& operation = std::get<Operation>(message.get_payload());
    // const DocumentID& document_id = sender->get_current_document();

    for (const auto& [client_id, client] : clients) {
        if (client == sender) {
            continue;
        }
        if (!client->has_current_document()) {
            continue;
        }
        if (client->get_current_document() != sender->get_current_document()) {
            continue;
        }
        if (client->is_syncing()) {
            pending_sync_operations[client_id].push_back(operation);
        } else if (client->is_live()){
            std::cout << "Routing to " << client_id << '\n';
            client->send(serialized);
        }
    }
}

void Server::open_document(std::shared_ptr<ClientConnection> client, const DocumentID& document_id) {
    if (document_id.empty()) {
        throw std::runtime_error("Document ID cannot be empty.");
    }
    if (!client->is_registered()) {
        throw std::runtime_error("Only registered clients can open documents.");
    }
    if (!document_store.exists(document_id)) {
        document_store.create_document(document_id);
    }

    PersistentDocument& document = document_store.get_document(document_id);
    auto history = document.get_history();

    client->set_current_document(document_id);
    client->mark_syncing();
    Message response(MessageType::SYNC_RESPONSE, "server", history);
    client->send(response.serialize());
}


// OperationLog Server::get_log() const {
//     return oplog;
// }