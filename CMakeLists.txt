cmake_minimum_required(VERSION 3.28)
cmake_policy(SET CMP0167 NEW)

project(GOOGLEDOCSPROJECT)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(main
    crdt_editor/app/main.cpp
    crdt_editor/src/document.cpp
    crdt_editor/crdt/id_generator.cpp
    crdt_editor/editor/cursor.cpp
    crdt_editor/editor/input_handler.cpp
    crdt_editor/editor/editor_command.cpp
    crdt_editor/src/operation_log.cpp
    crdt_editor/network/editor_session.cpp
    crdt_editor/network/fake_network.cpp
    crdt_editor/network/message.cpp
    crdt_editor/network/network_client.cpp
    crdt_editor/network/server.cpp
    crdt_editor/network/client_connection.cpp
    crdt_editor/network/framing.cpp
    crdt_editor/src/operation.cpp
    crdt_editor/src/elementID.cpp
    crdt_editor/src/character.cpp
    crdt_editor/tests/networking_tests.cpp
    crdt_editor/persistance/persistance_log.cpp
    crdt_editor/tests/persistance_tests.cpp
    crdt_editor/UI/editor_widget.cpp
    crdt_editor/tests/UI_tests.cpp
    crdt_editor/tests/CRDT_tests.cpp
    crdt_editor/UI/viewport.cpp
)

target_include_directories(main PRIVATE
    ${CMAKE_SOURCE_DIR}/crdt_editor
)

target_link_libraries(main 
    PRIVATE 
        Boost::boost
        ws2_32
        mswsock
        Qt6::Widgets
)
