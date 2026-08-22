#include <network/server.hpp>
#include <network/client_connection.hpp>

// create listening socket -> bind to port -> start listening -> begin accepting clients
Server::Server(boost::asio::io_context& io, unsigned short port): acceptor(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)), persistant_log("operations.log") {
    std::cout << "Server listening on port " << port << '\n';
    load_persistent_state();
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
                const auto& op = std::get<Operation>(message.get_payload());

                // if (applied_operations.contains(op)) { // will want to look into duplicate operations later
                //     return;
                // }
                // applied_operations.insert(op);
                oplog.record(op);
                persistant_log.record(op);
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

void Server::load_persistent_state() {
    for (const auto& operation : persistant_log.load()) {
        oplog.record(operation);
    }
}

void Server::finish_sync(std::shared_ptr<ClientConnection> client) {

    if (!client->is_syncing()) {
        return;
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
    Message response(MessageType::SYNC_RESPONSE, "server", oplog.get_log());
    connection->send(response.serialize());
}

void Server::route_cursor_update(const std::shared_ptr<ClientConnection>& sender, const Message& message) {

    const auto& update = std::get<CursorUpdate>(message.get_payload());
    presence.at(sender->get_client_id()).cursor = update.position;

    std::string serialized = message.serialize();

    for (const auto& [client_id, client] : clients) {
        if (client == sender) {
            continue;
        }
        if (client->is_live()) {
            std::cout << "Routing cursor to " << client_id << std::endl;
            client->send(serialized);
        }
    }
}

void Server::send_presence(const std::shared_ptr<ClientConnection>& client) {
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
    auto history = oplog.get_log();

    clients.emplace(client_id, connection); // probably want some try catch or other failsafe stuff
    pending_sync_operations.emplace(client_id, std::vector<Operation>{});
    presence.emplace(client_id, ClientPresence{ElementID(0, "__ROOT__")}); //Default sets it at ROOT

    connection->set_client_id(client_id);
    connection->mark_syncing();

    send_history(connection);
}

void Server::route_message(const std::shared_ptr<ClientConnection>& sender, const Message& message) {
    std::string serialized = message.serialize();

    const auto& operation = std::get<Operation>(message.get_payload());

    for (const auto& [client_id, client] : clients) {
        if (client == sender) {
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

OperationLog Server::get_log() const {
    return oplog;
}