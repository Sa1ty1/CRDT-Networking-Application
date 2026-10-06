#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <utility>
#include <limits>
#include <stdexcept>
#include <functional>
#include <network/editor_session.hpp>
#include <network/framing.hpp>
#include <persistance/document_id.hpp>
#include <util/thread_safe_queue.hpp>


enum class NetworkClientState {
    DISCONNECTED,
    CONNECTING,
    REGISTERING,
    REGISTERED,
    SYNCING,
    LIVE,
};

using StateChangeCallback = std::function<void(NetworkClientState)>;


class NetworkClient : public std::enable_shared_from_this<NetworkClient> {
public:

    using tcp = boost::asio::ip::tcp;

    // NetworkClient(boost::asio::io_context& io, EditorSession& session, std::string client_id);
    NetworkClient(boost::asio::io_context& io, std::string client_id,  ThreadSafeQueue<Message>& incoming_queue, ThreadSafeQueue<Message>& outgoing_queue);

    NetworkClientState get_state() const;

    void connect(const std::string& host, unsigned short port);

    tcp::socket& socket();

    void close_socket();

    void disconnect();

    void poll();

    void send_open_document(const DocumentID& document_id);

    void set_state_change_callback(StateChangeCallback callback);

    void notify_outgoing();

private:

    void connect_impl(const std::string& host, unsigned short port);
    void send_message_impl(std::string message);
    void send_open_document_impl(const DocumentID& document_id);
    void disconnect_impl();

    void send_hello();

    void set_state(NetworkClientState new_state);

    void start_read();

    void start_write();

    void handle_message(const std::string& serialized_message);

    void process_outgoing();

private:

    tcp::socket network_socket;
    std::string client_id;
    boost::asio::io_context& io;
    std::array<char, framing::HEADER_SIZE> read_header_buffer;
    std::vector<char> read_body_buffer;
    std::deque<std::shared_ptr<std::string>> write_queue;
    std::uint64_t connection_generation = 0;
    NetworkClientState state = NetworkClientState::DISCONNECTED;
    StateChangeCallback state_change_callback;
    ThreadSafeQueue<Message>& incoming_queue;
    ThreadSafeQueue<Message>& outgoing_queue;
};