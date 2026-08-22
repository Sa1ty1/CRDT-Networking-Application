#pragma once
#include <unordered_set>
#include <iostream>
#include <boost/asio.hpp>
#include <unordered_map>
#include <network/message.hpp>
#include <src/operation_log.hpp>
#include <persistance/persistance_log.hpp>

class ClientConnection;

struct ClientPresence {
    ElementID cursor;
};

class Server {
public:
    // create listening socket -> bind to port -> start listening -> begin accepting clients
    Server(boost::asio::io_context& io, unsigned short port);

    OperationLog get_log() const;

    void receive_message(std::shared_ptr<ClientConnection> sender, std::string serialized_message);

    void unregister_client(std::shared_ptr<ClientConnection> connection);

    void finish_sync(std::shared_ptr<ClientConnection> client);

    void load_persistent_state();

    void route_cursor_update(const std::shared_ptr<ClientConnection>& sender, const Message& message);

    void send_presence(const std::shared_ptr<ClientConnection>& client);

private:
    // begin asnychronously waiting for the next incoming TCP connection
    void accept_client();

    void send_history(std::shared_ptr<ClientConnection> connection);

    void register_client(std::shared_ptr<ClientConnection> connection, std::string client_id);

    void route_message(const std::shared_ptr<ClientConnection>& sender, const Message& message);

private:
    boost::asio::ip::tcp::acceptor acceptor; // listens at a port
    std::unordered_map<std::string, std::shared_ptr<ClientConnection>> clients;
    //std::unordered_set<std::string> syncing_clients;
    std::unordered_map<std::string, std::vector<Operation>> pending_sync_operations;
    OperationLog oplog;
    // std::unordered_set<Operation> applied_operations; // will want to look into duplicate operations later
    PersistentOperationLog persistant_log;
    std::unordered_map<std::string, ClientPresence> presence;
};

/*
Server server(io, 12345);
means conceptually listen on:
IPv4
port 12345
*/

// The Server should manage the collection of connections, 
// but each individual connection should manage its own socket operations.

/* Acceptor
Server starts
    ↓
bind to port
    ↓
listen
    ↓
wait for connection
*/