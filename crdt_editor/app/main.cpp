// Simple CLI playground

//Example usage:

// insert H
// insert e
// insert l
// insert l
// insert o

// show

// output: Hello

// Purpose:

// manual experimentation
// debugging
// #include <chrono>
// #include <stdio.h>
#include <iostream>
#include <QApplication>
// #include <random>
// #include <boost/asio.hpp>
// #include <cstdint>
// #include <cstring>
// #include <iostream>
// #include <src/document.hpp>
// #include <editor/input_handler.hpp>
// #include <src/operation_log.hpp>
// #include <network/server.hpp>
// #include <network/editor_session.hpp>
// #include <network/fake_network.hpp>
// #include <network/network_client.hpp>
#include <tests/networking_tests.hpp>
#include <tests/persistance_tests.hpp>
#include <tests/UI_tests.hpp>
#include <tests/CRDT_tests.hpp>
#include <UI/editor_widget.hpp>


int main(int argc, char* argv[]) {
    boost::asio::io_context io;

    QApplication app(argc, argv);

    Document document_a;
    Cursor cursor_a(ROOT_ID);
    Id_generator generator_a("client_a");
    InputHandler handler_a(document_a, cursor_a, generator_a);
    OperationLog log_a;

    EditorSession session_a(document_a, cursor_a, handler_a, log_a, generator_a);

    auto network_a = std::make_shared<NetworkClient>(io, session_a, "client_a");

    EditorWidget editor_a(session_a, *network_a);

    Document document_b;
    Cursor cursor_b(ROOT_ID);
    Id_generator generator_b("client_b");
    InputHandler handler_b(document_b, cursor_b, generator_b);
    OperationLog log_b;

    EditorSession session_b(document_b, cursor_b, handler_b, log_b, generator_b);

    auto network_b = std::make_shared<NetworkClient>(io, session_b, "client_b");

    EditorWidget editor_b(session_b, *network_b);

    Server server(io, 12345);

    editor_a.show();
    editor_b.show();

    network_a->connect("127.0.0.1", 12345);
    network_b->connect("127.0.0.1", 12345);

    return app.exec();
}




// int main(int argc, char* argv[]) {
//     QApplication app(argc, argv);
//     Document document;
//     Cursor cursor(ROOT_ID);
//     Id_generator generator("local");
//     InputHandler handler(document, cursor, generator);
//     OperationLog log;
//     EditorSession session(document, cursor, handler, log, generator);
//     EditorWidget editor(session);
//     editor.show();
//     return app.exec();
// }

// int main() {
//     row_col_test();
//     persistance_basic_test();
//     test_basic_connection_and_sync();
//     test_operation_routing();
//     test_late_client_sync();
//     test_disconnect_and_reconnect();
//     //test_duplicate_operation(); // need to think more about implementing this
//     test_duplicate_client_id();
//     test_hello_after_registration();
//     test_sync_complete_after_live();
//     test_operation_during_sync();
//     test_sync_complete_during_sync_is_valid();
//     insertion_in_middle_test();


//     std::cout << "All networking tests passed!\n";
// }

