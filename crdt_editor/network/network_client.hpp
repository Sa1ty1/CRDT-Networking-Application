#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <utility>
#include <limits>
#include <stdexcept>
#include <network/editor_session.hpp>
#include <network/framing.hpp>

enum class NetworkClientState {
    DISCONNECTED,
    CONNECTING,
    SYNCING,
    LIVE,
};

class NetworkClient : public std::enable_shared_from_this<NetworkClient> {
public:

    using tcp = boost::asio::ip::tcp;

    NetworkClient(boost::asio::io_context& io, EditorSession& session, std::string client_id);

    NetworkClientState get_state() const;

    void connect(const std::string& host, unsigned short port);

    void send_hello();

    void send_message(std::string message);

    tcp::socket& socket();

    void close_socket();

    void send_outgoing_operations();

    void disconnect();

    void send_cursor_update(const ElementID& position);

    void poll();

private:
    void start_read();

    void start_write();

    void handle_message(const std::string& serialized_message);

private:
    tcp::socket network_socket;
    std::string client_id;
    EditorSession& session;
    boost::asio::io_context& io;
    std::array<char, framing::HEADER_SIZE> read_header_buffer;
    std::vector<char> read_body_buffer;
    std::deque<std::shared_ptr<std::string>> write_queue;
    std::uint64_t connection_generation = 0;
    NetworkClientState state = NetworkClientState::DISCONNECTED;
};