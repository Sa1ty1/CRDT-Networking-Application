
#pragma once
#include <queue>
#include <src/document.hpp>
#include <editor/cursor.hpp>
#include <editor/input_handler.hpp>
#include <crdt/id_generator.hpp>
#include <network/message.hpp>


class EditorSession {
public:

    void handle_editor_command(EditorCommand command);

    std::vector<Operation> take_outgoing_operations();

    void receive_message(Message message);

    void flush_incoming();

    void apply(Message message);

    void apply_operation(const Operation& oper);

    void apply_history(const std::vector<Operation>& history);

    void receive_cursor_update(const std::string& client_id, const ElementID& position);

    void update_remote_cursors(const Operation& oper);

    std::string render();

    Document& get_doc() const;
    Cursor& get_cursor() const;
    const std::unordered_map<std::string, ElementID>& get_remote_cursors() const;

    EditorSession(Document& d, Cursor& c, InputHandler& hand, OperationLog& l, Id_generator& g);


private: // & means its a reference which means something else owns it. Since the client owns it this should not be a reference
    Document& doc;
    OperationLog& log;
    Cursor& cursor;
    Id_generator& gen; //maybe don't need this here; could be a stateless helper or free function
    InputHandler& handler;
    std::queue<Operation> outgoing_queue;
    std::queue<Message> incoming_queue;
    std::unordered_set<std::string> applied_operations;
    std::unordered_map<std::string, ElementID> remote_cursors;
};