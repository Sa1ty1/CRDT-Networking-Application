#include <network/network_client.hpp>

using tcp = boost::asio::ip::tcp;

NetworkClient::NetworkClient(boost::asio::io_context& io, std::string client_id,  ThreadSafeQueue<Message>& incoming_queue, ThreadSafeQueue<Message>& outgoing_queue): io(io), network_socket(io), client_id(std::move(client_id)), incoming_queue(incoming_queue), outgoing_queue(outgoing_queue) {}

// will need to change; violates multithreading invarient
NetworkClientState NetworkClient::get_state() const {
    return state;
}

void NetworkClient::connect(const std::string& host, unsigned short port) {
    auto self = shared_from_this();

    boost::asio::post(io, [self, host, port]() {
        self->connect_impl(host, port);
    });
}

void NetworkClient::connect_impl(const std::string& host, unsigned short port) {

    if (state != NetworkClientState::DISCONNECTED) {
        std::cerr << "Cannot connect: client is already connected or connecting" << std::endl;
        return;
    }

    // don't know if I need this
    boost::system::error_code ec;
    if (network_socket.is_open()) {
        network_socket.close();
    }
    network_socket.open(tcp::v4(), ec);
    if (ec) {
        std::cerr << "Socket open error: " << ec.message() << std::endl;
        set_state(NetworkClientState::DISCONNECTED);
        // state = NetworkClientState::DISCONNECTED;
        return;
    }

    set_state(NetworkClientState::CONNECTING);
    //state = NetworkClientState::CONNECTING;
    const std::uint64_t generation = ++connection_generation;

    auto self = shared_from_this();

    auto resolver = std::make_shared<tcp::resolver>(network_socket.get_executor());

    resolver->async_resolve(host, std::to_string(port), [self, resolver, generation](
        const boost::system::error_code& ec,
        tcp::resolver::results_type results) {
            if (generation != self->connection_generation) {
                return;
            }
            if (ec) {
                std::cerr << "Resolve error: " << ec.message() << std::endl;
                self->set_state(NetworkClientState::DISCONNECTED);
                // self->state = NetworkClientState::DISCONNECTED;
                return;
            }
            boost::asio::async_connect(
                self->network_socket, results,
                [self, generation](const boost::system::error_code& ec,
                const tcp::endpoint&) {
                    if (generation != self->connection_generation) {
                        return;
                    }
                    if (ec) {
                        std::cerr << "Connect error: " << ec.message() << std::endl;
                        self->set_state(NetworkClientState::DISCONNECTED);
                        // self->state = NetworkClientState::DISCONNECTED;
                        ++self->connection_generation;
                        self->close_socket();
                        return;
                    }
                    self->set_state(NetworkClientState::REGISTERING);
                    //self->state = NetworkClientState::REGISTERED;
                    std::cout << "Connected to server" << std::endl;
                    self->send_hello();
                    self->start_read();
                }
            );
        }
    );
}

void NetworkClient::disconnect() {
    auto self = shared_from_this();

    boost::asio::post(io, [self]() {
        self->disconnect_impl();
    });
}

void NetworkClient::disconnect_impl() {
    if (state == NetworkClientState::DISCONNECTED) {
        return;
    }
    set_state(NetworkClientState::DISCONNECTED);
    // state = NetworkClientState::DISCONNECTED;
    ++connection_generation; // invalidate callbacks belonging to the old connection
    
    close_socket();

    write_queue.clear();

    std::cout << "Network client disconnected" << std::endl;
}

void NetworkClient::send_hello() {
    Message hello(MessageType::HELLO, client_id, std::monostate{});
    std::cout << "Sending HELLO from " << client_id << '\n';
    send_message_impl(hello.serialize());
}

void NetworkClient::send_message_impl(std::string message) {
    if (state == NetworkClientState::DISCONNECTED || state == NetworkClientState::CONNECTING) {
        std::cerr << "Cannot send message: client is not connected" << std::endl;
        return;
    }
    //bool write_in_progress = !write_queue.empty();
    auto framed_message = std::make_shared<std::string>(framing::frame_message(std::move(message)));
    
    bool start = write_queue.empty();
    write_queue.push_back(std::move(framed_message));
    if (start) {
        start_write();
    }
}

tcp::socket& NetworkClient::socket() {
    return network_socket;
}

void NetworkClient::close_socket() {
    boost::system::error_code ec;
    network_socket.cancel(ec);
    network_socket.shutdown(tcp::socket::shutdown_both, ec);
    network_socket.close(ec);
}

void NetworkClient::poll() {
    io.poll();
}

void NetworkClient::send_open_document(const DocumentID& document_id) {
    auto self = shared_from_this();

    boost::asio::post(io, [self, document_id] () {
        self->send_open_document_impl(document_id);
    });
}


void NetworkClient::send_open_document_impl(const DocumentID& document_id) {
    if (state != NetworkClientState::REGISTERED) {
        std::cerr << "Cannot open document: client is not registered" << std::endl;
        return;
    }
    if (document_id.empty()) {
        std::cerr << "Cannot open document: empty document ID" << std::endl;
        return;
    }

    OpenDocument request{document_id};

    Message message(MessageType::OPEN_DOCUMENT, client_id, request);
    set_state(NetworkClientState::SYNCING);
    //state = NetworkClientState::SYNCING;
    send_message_impl(message.serialize());

}

void NetworkClient::set_state_change_callback(StateChangeCallback callback) {
    state_change_callback = std::move(callback);
}

void NetworkClient::set_state(NetworkClientState new_state) {
    if (state == new_state) {
        return;
    }
    state = new_state;
    if (state_change_callback) {
        state_change_callback(state);
    }
}


void NetworkClient::start_read() {
    auto self = shared_from_this();
    const std::uint64_t generation = connection_generation;

    boost::asio::async_read(
        network_socket, boost::asio::buffer(read_header_buffer),
        [self, generation](const boost::system::error_code& ec, std::size_t bytes) {
            if (generation != self->connection_generation) {
                return;
            }
            if (ec) {
                if (self->state != NetworkClientState::DISCONNECTED) {
                    std::cerr << "Read error: " << ec.message() << std::endl;
                    self->disconnect();
                }
                return;
            }
            std::uint32_t body_length = framing::decode_length(self->read_header_buffer);
            if (body_length > framing::MAX_MESSAGE_SIZE) {
                std::cerr << "Message too large\n";
                self->disconnect();
                return;
            }
            self->read_body_buffer.resize(body_length);

            boost::asio::async_read(
                self->network_socket,
                boost::asio::buffer(self->read_body_buffer),
                [self, generation](const boost::system::error_code& ec, std::size_t bytes) {
                    if (generation != self->connection_generation) {
                        return;
                    }
                    if (ec) {
                        if (self->state != NetworkClientState::DISCONNECTED) {
                            std::cerr << "Read error: " << ec.message() << std::endl;
                            self->disconnect();
                        }
                        return;
                    }
                    std::string message(self->read_body_buffer.begin(), self->read_body_buffer.end());
                    self->handle_message(message);
                    if (self->state != NetworkClientState::DISCONNECTED) {
                        self->start_read();
                    }
                }
            );
        }
    );
}


void NetworkClient::start_write() {
    auto self = shared_from_this();
    const std::uint64_t generation = connection_generation;

    boost::asio::async_write(
        network_socket, boost::asio::buffer(*write_queue.front()),
        [self, generation](const boost::system::error_code& ec, std::size_t bytes) {
            if (generation != self->connection_generation) {
                return;
            }
            if (ec) {
                if (self->state != NetworkClientState::DISCONNECTED) {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                    self->disconnect();
                }
                return;
            }
            self->write_queue.pop_front();
            if (!self->write_queue.empty() && self->state != NetworkClientState::DISCONNECTED) {
                self->start_write();
            }
        }
    );
}

void NetworkClient::handle_message(const std::string& serialized_message) {
    
    try {
        Message message = Message::deserialize(serialized_message);

        switch(message.get_type()) {
            case MessageType::OPERATION: {
                if (state != NetworkClientState::SYNCING && state != NetworkClientState::LIVE) {
                    std::cerr << "Unexpected OPERATION from server" << std::endl;
                    disconnect();
                    return;
                }
                // session.receive_message(std::move(message));
                incoming_queue.push(std::move(message));
                break;
            }
            case MessageType::SYNC_RESPONSE: {
                if (state != NetworkClientState::SYNCING) {
                    std::cerr << "Unexpected SYNC_RESPONSE from server" << std::endl;
                    disconnect();
                    return;
                }

                incoming_queue.push(std::move(message));
                break;
            }
            case MessageType::HELLO: {
                std::cerr << "Unexpected HELLO from server" << std::endl;
                disconnect();
                return;
                //client shouldn't normally revieve this
                //break;
            }
            case MessageType::HELLO_ACK: {
                if (state != NetworkClientState::REGISTERING) {
                    std::cerr << "Unexpected HELLO_ACK from server" << std::endl;
                    disconnect();
                    return;
                }
                std::cout << "Registration confirmed by server" << std::endl;
                set_state(NetworkClientState::REGISTERED);
                break;
            }
            case MessageType::SYNC_COMPLETE: {
                // client shouldn't normally recieve this
                std::cerr << "Unexpected message from server" << std::endl;
                disconnect();
                break;
            }
            case MessageType::CURSOR_UPDATE: {
                if (state != NetworkClientState::SYNCING && state != NetworkClientState::LIVE) {
                    std::cerr << "Unexpected CURSOR_UPDATE from server" << std::endl;
                    disconnect();
                    return;
                }
                // const auto& update = std::get<CursorUpdate>(message.get_payload());
                // session.receive_cursor_update(message.get_sender(), update.position);
                incoming_queue.push(std::move(message));
                break;
            }
            default: {
                std::cerr << "Unexpected message type from server" << std::endl;
                disconnect();
                return;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Invalid message from server: " << e.what() << std::endl;
        disconnect();
    }
}

void NetworkClient::notify_outgoing() {
    auto self = shared_from_this();

    boost::asio::post(io, [self]() {
        self->process_outgoing();
    });
}


void NetworkClient::process_outgoing() {
    auto messages = outgoing_queue.take_all();
    for (auto& message : messages) {
        send_message_impl(message.serialize());
    }

}