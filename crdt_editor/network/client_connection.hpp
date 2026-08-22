#pragma once
#include <cstring>
#include <iostream>
#include <queue>
#include <cstdint>
#include <network/server.hpp>
#include <network/framing.hpp>

// Responsibility
/*
ClientConnection
│
├── owns one socket
│
├── reads from that socket
│
├── writes to that socket
│
└── reports messages to Server
*/

enum class ClientState {
    UNREGISTERED,
    SYNCING,
    LIVE,
    DISCONNECTED
};

class ClientConnection : public std::enable_shared_from_this<ClientConnection> {
public:

    using tcp = boost::asio::ip::tcp;

    ClientConnection(boost::asio::any_io_executor executor, Server& server);

    tcp::socket& socket();

    void start();

    void send(std::string message);

    void disconnect();

    std::string get_client_id() const;
    void set_client_id(std::string id);
    ClientState get_state() const;
    bool is_registered() const;
    bool is_syncing() const;
    bool is_live() const;
    void mark_syncing();
    void mark_live();
    void mark_disconnected();

private:

    void read_header();

    void read_body();

    void process_message(std::string message);

    void start_write();


private:

    tcp::socket client_socket;

    Server& server;

    std::string client_id;

    std::array<char, framing::HEADER_SIZE> read_header_buffer; // replace with proper, protocol-specific header type

    std::vector<char> read_body_buffer;

    std::deque<std::string> write_queue; //outgoing message state

    ClientState state = ClientState::UNREGISTERED;

};

//The Server should not need to know how the socket reads bytes.
/*
ClientConnection
        │
        │ "I received this Message"
        ▼
Server
        │
        ▼
route_message()
*/