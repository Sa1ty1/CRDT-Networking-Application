#include <network/editor_session.hpp>


void EditorSession::handle_editor_command(EditorCommand command) {
    std::cout << "SESSION COMMAND: " << command.get_type() << '\n';
    std::vector<Operation> actions = handler.process_command(command);

    for (const auto& action: actions) {
        apply_operation(action, true);
        Message message(MessageType::OPERATION, client_id, action);
        // outgoing_queue.push(std::move(message));
        queue_outgoing_message(message);
    }
    if (command.get_type() != MoveUp && command.get_type() != MoveDown) {
        cursor.update_desired_column(doc);
    }
    std::cout << "done\n";
}

void EditorSession::receive_cursor_update(const std::string& client_id, const ElementID& position) {
    std::cout << "RECEIVED CURSOR UPDATE: " << client_id << " -> " << position.to_String() << '\n';
    
    auto resolved = doc.resolve_cursor_anchor(position);
    if (!resolved) {
        pending_cursor_updates.insert_or_assign(client_id, position);
        return;
    }
    remote_cursors.insert_or_assign(client_id, position);
    // If this client had an older pending update, this one supersedes it.
    pending_cursor_updates.erase(client_id);    
}

// void EditorSession::receive_message(Message message) {
//     incoming_queue.push(message);        
// }

void EditorSession::flush_incoming() {
    auto messages = incoming_queue.take_all();
    for (auto& message : messages) {
        apply(std::move(message));
    }
}

void EditorSession::apply_history(const std::vector<Operation>& history){
    for(const auto& op : history) {
        std::visit([&](auto const& operation) {
            using T = std::decay_t<decltype(operation)>;
            if constexpr (std::is_same_v<T, InsertOperation>) {
                gen.sync_clock(operation.get_id().get_lamport());
            }
        }, op);
        
        apply_operation(op, false);
    }

    update_pending_cursor_updates();
}

void EditorSession::apply(Message message) {

    switch (message.get_type()) {
        case MessageType::OPERATION: {
            const Operation& oper = std::get<Operation>(message.get_payload());

            std::visit([&](auto const& op) {
                using T = std::decay_t<decltype(op)>;
                if constexpr (std::is_same_v<T, InsertOperation>) {
                    gen.sync_clock(op.get_id().get_lamport());
                }
            }, oper);

            apply_operation(oper, false);
            update_pending_cursor_updates();
            break;
        }
        case MessageType::SYNC_RESPONSE: {
            const auto& history = std::get<std::vector<Operation>>(message.get_payload());
            apply_history(history);
            Message sync_complete(MessageType::SYNC_COMPLETE, client_id, std::monostate{});
            // outgoing_queue.push(std::move(sync_complete));
            queue_outgoing_message(sync_complete);

            Message cursor_update(MessageType::CURSOR_UPDATE, client_id, CursorUpdate{cursor.get_anchor()});
            // outgoing_queue.push(std::move(cursor_update));
            queue_outgoing_message(cursor_update);

            break;
        }
        case MessageType::CURSOR_UPDATE: {
            const CursorUpdate& update = std::get<CursorUpdate>(message.get_payload());

            receive_cursor_update(message.get_sender(), update.position);
            break;
        }
        default:
            break;
    }
}

void EditorSession::queue_outgoing_message(Message message) {
    outgoing_queue.push(std::move(message));
    if (outgoing_message_callback) {
        outgoing_message_callback();
    }
}


void EditorSession::queue_cursor_update() {
    Message cursor_update(MessageType::CURSOR_UPDATE, client_id, CursorUpdate{cursor.get_anchor()});
    // outgoing_queue.push(std::move(cursor_update));
    queue_outgoing_message(cursor_update);
}

void EditorSession::apply_operation(const Operation& oper, bool update_cursor) {
    std::string serialized = operation_serializer::serialize(oper);

    if (!applied_operations.insert(serialized).second) {
        std::cout << "DUPLICATE OPERATION: " << serialized << std::endl;
        return;
    }

    log.record(oper);
    doc.apply(oper);

    if (update_cursor) {
        cursor.update(doc, oper);
    } else {
        cursor.update_on_remote(doc, oper);
    }

    update_remote_cursors(oper);
    //update_pending_cursor_updates();
}

void EditorSession::update_remote_cursors(const Operation& oper) {
    std::visit([&](const auto& operation) {
        using T = std::decay_t<decltype(operation)>;

        if constexpr (std::is_same_v<T, RemoveOperation>) {
            const ElementID& target = operation.get_target();

            std::cout << "REMOTE CURSOR DELETE UPDATE\n";

            for (auto& [client_id, anchor] : remote_cursors) {
                
                if (anchor == target) {
                    ElementID vis_predecessor = doc.visible_predecessor(target);
                    anchor = vis_predecessor;
                }
            }
        }
    }, oper);
}

void EditorSession::update_pending_cursor_updates() {
    for (auto it = pending_cursor_updates.begin(); it != pending_cursor_updates.end();) {
        const std::string& client_id = it->first;
        const ElementID& position = it->second;
        auto resolved = doc.resolve_cursor_anchor(position);
        if (resolved) {
            std::cout << "RESOLVED PENDING CURSOR: " << client_id << " -> " << position.to_String() << " (resolved to " << resolved->to_String() << ")\n";
            remote_cursors.insert_or_assign(client_id, position);
            it = pending_cursor_updates.erase(it);
        } else {
            ++it;
        }
    }
}

std::string EditorSession::render() {
    return doc.render();
}

Document& EditorSession::get_doc() const {
    return doc;
}

Cursor& EditorSession::get_cursor() const {
    return cursor;
}

const std::unordered_map<std::string, ElementID>& EditorSession::get_remote_cursors() const {
    return remote_cursors;
}


EditorSession::EditorSession(Document& d, Cursor& c, InputHandler& hand, OperationLog& l, Id_generator& g, std::string client_id, ThreadSafeQueue<Message>& incoming_queue, ThreadSafeQueue<Message>& outgoing_queue, OutgoingMessageCallback outgoing_message_callback): doc(d), cursor(c), handler(hand), log(l), gen(g), client_id(std::move(client_id)), incoming_queue(incoming_queue), outgoing_queue(outgoing_queue), outgoing_message_callback(outgoing_message_callback) {}