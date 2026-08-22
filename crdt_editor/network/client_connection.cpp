#include <network/client_connection.hpp>

    using tcp = boost::asio::ip::tcp;

    ClientConnection::ClientConnection(boost::asio::any_io_executor executor, Server& server) : client_socket(executor), server(server) {}

    tcp::socket& ClientConnection::socket() {
        return client_socket;
    }

    std::string ClientConnection::get_client_id() const { return client_id;}
    void ClientConnection::set_client_id(std::string id) { client_id = std::move(id);}
    ClientState ClientConnection::get_state() const { return state;}
    bool ClientConnection::is_registered() const {return state == ClientState::SYNCING || state == ClientState::LIVE;}
    bool ClientConnection::is_syncing() const { return state == ClientState::SYNCING;}
    bool ClientConnection::is_live() const { return state == ClientState::LIVE;}
    void ClientConnection::mark_syncing() { state = ClientState::SYNCING;}
    void ClientConnection::mark_live() { state = ClientState::LIVE;}
    void ClientConnection::mark_disconnected() { state = ClientState::DISCONNECTED;}

    void ClientConnection::start() {
        std::cout << "Client Connected" << std::endl;
        read_header();
        // might also do begin reading and client auth/reg
    }

    void ClientConnection::send(std::string message) {
        if (state == ClientState::UNREGISTERED || state == ClientState::DISCONNECTED) {
            return;
        }
        bool already_writing = !write_queue.empty();
        write_queue.push_back(framing::frame_message(std::move(message)));
        if (!already_writing) {
            start_write();
        }
    }


    void ClientConnection::read_header() {
        auto self = shared_from_this();

        // async_read keeps reading until the requested number of bytes have arrived
        boost::asio::async_read(client_socket, boost::asio::buffer(read_header_buffer), [self](const boost::system::error_code& ec, std::size_t bytes) {
            if (ec) {
                self->disconnect();
                return;
            }
            std::uint32_t body_length = framing::decode_length(self->read_header_buffer);

            if (body_length > framing::MAX_MESSAGE_SIZE) {
                std::cerr << "Message too large" << std::endl;
                self->disconnect();
                return;
            }
            self->read_body_buffer.resize(body_length);
            self->read_body();
        });
    }

    void ClientConnection::read_body() {
        auto self = shared_from_this();

        boost::asio::async_read(client_socket, boost::asio::buffer(read_body_buffer),
            [self](const boost::system::error_code& ec, std::size_t bytes) {
                if (ec) {
                    self->disconnect();
                    return;
                }
                std::string message(self->read_body_buffer.begin(),
                                    self->read_body_buffer.end());

                self->process_message(std::move(message));
            }
        );
    }

    void ClientConnection::process_message(std::string message) {
        //server.receive_message(shared_from_this(), std::move(message));
        //for now, print
        std::cout << "Client Connection Received: " << message << '\n';
        server.receive_message(shared_from_this(), std::move(message));
        if (state != ClientState::DISCONNECTED) {
            read_header();
        }
    }

    void ClientConnection::start_write() {
        auto self = shared_from_this();

        boost::asio::async_write(client_socket, boost::asio::buffer(write_queue.front()), [self](const boost::system::error_code& ec, std::size_t bytes) {
            if (ec) {
                self->disconnect();
                return;
            }
            self->write_queue.pop_front();

            if (!self->write_queue.empty() && self->state != ClientState::DISCONNECTED) {
                self->start_write();
            }
        });
    }

    void ClientConnection::disconnect() {
        if (state == ClientState::DISCONNECTED) {
            return;
        }

        mark_disconnected();

        if (!client_id.empty()) {
            server.unregister_client(shared_from_this());
        }

        boost::system::error_code ec;

        client_socket.cancel(ec);

        client_socket.shutdown(tcp::socket::shutdown_both, ec);

        client_socket.close(ec);

        write_queue.clear();

        std::cout << "Client disconnected" << std::endl;
    }
