#include <tests/networking_tests.hpp>

// void run_for(boost::asio::io_context& io, std::chrono::milliseconds duration) {
//     auto work = boost::asio::make_work_guard(io);

//     std::thread io_thread([&io]() {
//         io.run();
//     });

//     std::this_thread::sleep_for(duration);

//     work.reset();
//     io.stop();

//     io_thread.join();
// }

void run_io_until(boost::asio::io_context& io, const std::function<bool()>& condition) {
    for (int i = 0; i < 1000 && !condition(); ++i) {
        run_io(io);
    }

    assert(condition());
}

void run_io(boost::asio::io_context& io) {
    io.restart();
    io.run_one();
}

void test_basic_connection_and_sync() {
    std::cout << "\n--- test_basic_connection_and_sync ---\n";

    boost::asio::io_context io;

    Server server(io, 12345);

    //run_io(io);

    // Create the sessions needed by the clients.
    Document document;
    Cursor cursor(ROOT_ID);
    Id_generator generator("client");
    InputHandler handler(document, cursor, generator);
    OperationLog log;
    EditorSession session(document, cursor, handler, log, generator);

    auto client = std::make_shared<NetworkClient>(io, session, "client");

    client->connect("127.0.0.1", 12345);

    run_io_until(io, [&] {
        return client->get_state() == NetworkClientState::LIVE;
    });

    /*
     * At this point the entire connection sequence should have happened:
     *
     * NetworkClient:
     *
     * DISCONNECTED
     *      ↓
     * CONNECTING
     *      ↓
     * SYNCING
     *      ↓
     * LIVE
     *
     * Server:
     *
     * ClientConnection:
     * UNREGISTERED
     *      ↓
     * SYNCING
     *      ↓
     * LIVE
     */

    //assert(client->get_state() == NetworkClientState::LIVE);

    std::cout << "PASSED\n";
}

void test_operation_routing() {
    std::cout << "\n--- test_operation_routing ---\n";

    boost::asio::io_context io;

    Server server(io, 12345);

    Document documentA;
    Cursor cursorA(ROOT_ID);
    Id_generator generatorA("clientA");
    InputHandler handlerA(documentA, cursorA, generatorA);
    OperationLog logA;
    EditorSession sessionA(documentA, cursorA, handlerA, logA, generatorA);
    
    Document documentB;
    Cursor cursorB(ROOT_ID);
    Id_generator generatorB("clientB");
    InputHandler handlerB(documentB, cursorB, generatorB);
    OperationLog logB;
    EditorSession sessionB(documentB, cursorB, handlerB, logB, generatorB);

    auto clientA = std::make_shared<NetworkClient>(io, sessionA, "clientA");

    auto clientB = std::make_shared<NetworkClient>(io, sessionB, "clientB");


    clientA->connect("127.0.0.1", 12345);
    clientB->connect("127.0.0.1", 12345);

    // Wait for both clients to become LIVE.
    run_io_until(io, [&] {
        return clientA->get_state() == NetworkClientState::LIVE;
    });

    run_io_until(io, [&] {
        return clientB->get_state() == NetworkClientState::LIVE;
    });
    // Perform an edit through A.
    sessionA.handle_editor_command(EditorCommand(InsertCharacter, 'A'));

    clientA->send_outgoing_operations();


    run_io_until(io, [&] {
        sessionB.flush_incoming();
        return sessionA.get_doc().render() == sessionB.get_doc().render();
    });

    // Verify B received the operation.

    std::cout << "PASSED\n";
}

void test_late_client_sync() {
    std::cout << "\n--- test_late_client_sync ---\n";

    boost::asio::io_context io;

    Server server(io, 12345);

    Document documentA;
    Cursor cursorA(ROOT_ID);
    Id_generator generatorA("clientA");
    InputHandler handlerA(documentA, cursorA, generatorA);
    OperationLog logA;
    EditorSession sessionA(documentA, cursorA, handlerA, logA, generatorA);
    
    Document documentB;
    Cursor cursorB(ROOT_ID);
    Id_generator generatorB("clientB");
    InputHandler handlerB(documentB, cursorB, generatorB);
    OperationLog logB;
    EditorSession sessionB(documentB, cursorB, handlerB, logB, generatorB);

    auto clientA = std::make_shared<NetworkClient>(io, sessionA, "clientA");

    auto clientB = std::make_shared<NetworkClient>(io, sessionB, "clientB");

    // ------------------------------------------------------------
    // A connects first
    // ------------------------------------------------------------

    clientA->connect("127.0.0.1", 12345);

    run_io_until(io, [&] {
        return clientA->get_state() == NetworkClientState::LIVE;
    });

    // ------------------------------------------------------------
    // A creates several operations
    // ------------------------------------------------------------

    sessionA.handle_editor_command(EditorCommand(InsertCharacter, 'A'));
    sessionA.handle_editor_command(EditorCommand(InsertCharacter, 'B'));
    sessionA.handle_editor_command(EditorCommand(InsertCharacter, 'C'));

    clientA->send_outgoing_operations();

    run_io(io);

    // ------------------------------------------------------------
    // B connects after those operations already exist
    // ------------------------------------------------------------

    clientB->connect("127.0.0.1", 12345);

    /*
     * B should have:
     *
     *     SYNC_RESPONSE
     *          ↓
     *     apply_history()
     *          ↓
     *     SYNC_COMPLETE
     *          ↓
     *     LIVE
     */
    
    run_io_until(io, [&] {
        return clientB->get_state() == NetworkClientState::LIVE;
    });

    // Verify B has A's existing document.
    run_io_until(io, [&] {
        sessionB.flush_incoming();
        return sessionA.get_doc().render() == sessionB.get_doc().render();
    });

    std::cout << "PASSED\n";
}

void test_disconnect_and_reconnect() {
    std::cout << "\n--- test_disconnect_and_reconnect ---\n";

    boost::asio::io_context io;

    Server server(io, 12345);

    Document documentA;
    Cursor cursorA(ROOT_ID);
    Id_generator generatorA("clientA");
    InputHandler handlerA(documentA, cursorA, generatorA);
    OperationLog logA;
    EditorSession sessionA(documentA, cursorA, handlerA, logA, generatorA);

    auto clientA = std::make_shared<NetworkClient>(io, sessionA, "clientA");

    // ------------------------------------------------------------
    // Initial connection
    // ------------------------------------------------------------

    clientA->connect("127.0.0.1", 12345);

    run_io_until(io, [&] {
        return clientA->get_state() == NetworkClientState::LIVE;
    });

    // ------------------------------------------------------------
    // Disconnect
    // ------------------------------------------------------------

    clientA->disconnect();

    assert(clientA->get_state() == NetworkClientState::DISCONNECTED);

    // ------------------------------------------------------------
    // Reconnect
    // ------------------------------------------------------------

    clientA->connect("127.0.0.1", 12345);

    run_io_until(io, [&] {
        return clientA->get_state() == NetworkClientState::LIVE;
    });

    std::cout << "PASSED\n";
}

void test_duplicate_operation() {
    std::cout << "\n--- test_duplicate_operation ---\n";

    boost::asio::io_context io;

    Server server(io, 12345);

    Document documentA;
    Cursor cursorA(ROOT_ID);
    Id_generator generatorA("clientA");
    InputHandler handlerA(documentA, cursorA, generatorA);
    OperationLog logA;
    EditorSession sessionA(documentA, cursorA, handlerA, logA, generatorA);
    
    Document documentB;
    Cursor cursorB(ROOT_ID);
    Id_generator generatorB("clientB");
    InputHandler handlerB(documentB, cursorB, generatorB);
    OperationLog logB;
    EditorSession sessionB(documentB, cursorB, handlerB, logB, generatorB);

    auto clientA = std::make_shared<NetworkClient>(io, sessionA, "clientA");

    auto clientB = std::make_shared<NetworkClient>(io, sessionB, "clientB");

    clientA->connect("127.0.0.1", 12345);
    clientB->connect("127.0.0.1", 12345);

    run_io_until(io, [&] {
        return clientA->get_state() == NetworkClientState::LIVE;
    });
    run_io_until(io, [&] {
        return clientB->get_state() == NetworkClientState::LIVE;
    });


    // Create exactly one operation.
    //
    sessionA.handle_editor_command(EditorCommand(InsertCharacter, 'A'));

    auto operations = sessionA.take_outgoing_operations();

    assert(operations.size() == 1);

    Operation operation = operations.front();

    Message message(
        MessageType::OPERATION,
        "clientA",
        operation
    );

    // Send the exact same operation twice.
    clientA->send_message(message.serialize());
    clientA->send_message(message.serialize());

    // run_io_until(io, [&] {
    //     return server.get_received_operation_count() == 2;
    // });

    /*
     * Eventually this should be true:
     *
     * server.operation_log.size() == 1
     *
     * rather than:
     *
     * server.operation_log.size() == 2
     */

    assert(server.get_log().size() == 1);
    
    std::cout << "PASSED\n";
}

void test_duplicate_client_id() {
    std::cout << "\n--- test_duplicate_client_id ---\n";

    boost::asio::io_context io;

    Server server(io, 12345);

    // ------------------------------------------------------------
    // Client A
    // ------------------------------------------------------------

    Document documentA;
    Cursor cursorA(ROOT_ID);
    Id_generator generatorA("client");
    InputHandler handlerA(documentA, cursorA, generatorA);
    OperationLog logA;
    EditorSession sessionA(documentA, cursorA, handlerA, logA, generatorA);

    auto clientA =
        std::make_shared<NetworkClient>(io, sessionA, "client");

    clientA->connect("127.0.0.1", 12345);

    run_io_until(io, [&] {
        return clientA->get_state() == NetworkClientState::LIVE;
    });

    // ------------------------------------------------------------
    // Client B attempts to use the same client ID.
    // ------------------------------------------------------------

    Document documentB;
    Cursor cursorB(ROOT_ID);
    Id_generator generatorB("client");
    InputHandler handlerB(documentB, cursorB, generatorB);
    OperationLog logB;
    EditorSession sessionB(documentB, cursorB, handlerB, logB, generatorB
    );

    auto clientB =std::make_shared<NetworkClient>(io, sessionB, "client");

    clientB->connect("127.0.0.1", 12345);

    run_io_until(io, [&] {
        return clientB->get_state() == NetworkClientState::DISCONNECTED;
    });

    run_io_until(io, [&] {
        return clientA->get_state() == NetworkClientState::LIVE;
    });

    std::cout << "PASSED" << std::endl;

}

void test_hello_after_registration() {
    std::cout << "\n--- test_hello_after_registration ---\n";
    boost::asio::io_context io;

    Server server(io, 12345);

    Document document;
    Cursor cursor(ROOT_ID);
    Id_generator generator("client");
    InputHandler handler(document, cursor, generator);
    OperationLog log;

    EditorSession session(document, cursor, handler, log, generator);

    auto client = std::make_shared<NetworkClient>(io, session, "client");

    client->connect("127.0.0.1", 12345);

    run_io_until(io, [&] {
        return client->get_state() == NetworkClientState::LIVE;
    });

    client->send_hello();

    run_io_until(io, [&] {
        return client->get_state() == NetworkClientState::DISCONNECTED;
    });

    std::cout << "PASSED" << std::endl;
}

void test_sync_complete_after_live() {
    std::cout << "\n--- test_sync_complete_after_live ---\n";

    boost::asio::io_context io;

    Server server(io, 12345);

    Document document;
    Cursor cursor(ROOT_ID);
    Id_generator generator("client");
    InputHandler handler(document, cursor, generator);
    OperationLog log;

    EditorSession session(
        document,
        cursor,
        handler,
        log,
        generator
    );

    auto client = std::make_shared<NetworkClient>(io, session, "client");

    client->connect("127.0.0.1", 12345);

    run_io_until(io, [&] {
        return client->get_state() == NetworkClientState::LIVE;
    });

    Message complete(
        MessageType::SYNC_COMPLETE,
        "client",
        std::monostate{}
    );

    client->send_message(complete.serialize());

    run_io_until(io, [&] {
        return client->get_state() == NetworkClientState::DISCONNECTED;
    });

    std::cout << "PASSED" << std::endl;
}

void test_operation_during_sync() {
    std::cout << "\n--- test_operation_during_sync ---\n";

    boost::asio::io_context io;

    Server server(io, 12345);

    Document document;
    Cursor cursor(ROOT_ID);
    Id_generator generator("client");
    InputHandler handler(document, cursor, generator);
    OperationLog log;

    EditorSession session(
        document,
        cursor,
        handler,
        log,
        generator
    );

    auto client = std::make_shared<NetworkClient>(io, session, "client");

    session.handle_editor_command(
        EditorCommand(InsertCharacter, 'A')
    );

    auto operations = session.take_outgoing_operations();

    assert(operations.size() == 1);

    Message operation_message(
        MessageType::OPERATION,
        "client",
        operations.front()
    );

    client->connect("127.0.0.1", 12345);

    while (client->get_state() == NetworkClientState::DISCONNECTED ||
        client->get_state() == NetworkClientState::CONNECTING) {
        io.poll_one();
    }

    assert(client->get_state() == NetworkClientState::SYNCING);

    client->send_message(operation_message.serialize());

    run_io_until(io, [&] {
        return client->get_state() == NetworkClientState::DISCONNECTED;
    });

    std::cout << "PASSED" << std::endl;
}

void test_sync_complete_during_sync_is_valid() {
    std::cout << "\n--- test_sync_complete_during_sync_is_valid ---\n";

    boost::asio::io_context io;

    Server server(io, 12345);

    Document document;
    Cursor cursor(ROOT_ID);
    Id_generator generator("client");
    InputHandler handler(document, cursor, generator);
    OperationLog log;

    EditorSession session(
        document,
        cursor,
        handler,
        log,
        generator
    );

    auto client =
        std::make_shared<NetworkClient>(io, session, "client");

    client->connect("127.0.0.1", 12345);

    // Advance until the client has connected and entered SYNCING.
    while (client->get_state() == NetworkClientState::DISCONNECTED ||
        client->get_state() == NetworkClientState::CONNECTING) {
        io.poll_one();
    }

    assert(client->get_state() == NetworkClientState::SYNCING);

    Message complete(
        MessageType::SYNC_COMPLETE,
        "client",
        std::monostate{}
    );

    client->send_message(complete.serialize());

    run_io_until(io, [&] {
        return client->get_state() == NetworkClientState::LIVE;
    });

    std::cout << "PASSED" << std::endl;
}
