#include <network/editor_session.hpp>


void EditorSession::handle_editor_command(EditorCommand command) {
    std::cout << "process_command\n";
    std::cout << "SESSION COMMAND: " << command.get_type() << '\n';
    std::vector<Operation> actions = handler.process_command(command);
    std::cout << "record/apply loop\n";

    for (const auto& action: actions) {
        apply_operation(action, true);
        outgoing_queue.push(action);
    }
    if (command.get_type() != MoveUp && command.get_type() != MoveDown) {
        cursor.update_desired_column(doc);
    }
    std::cout << "done\n";
}

std::vector<Operation> EditorSession::take_outgoing_operations() {
    std::vector<Operation> opers;
    while (!outgoing_queue.empty()) {
        opers.emplace_back(outgoing_queue.front());
        outgoing_queue.pop();
    }
    return opers;
}

void EditorSession::receive_cursor_update(const std::string& client_id, const ElementID& position) {
    std::cout << "RECEIVED CURSOR UPDATE: " << client_id << " -> " << position.to_String() << '\n';
    
    auto resolved = doc.resolve_cursor_anchor(position);
    if (!resolved) {
        std::cout << "  cannot resolve cursor\n";
        return;
    }
    std::cout << "  resolved to: " << resolved->to_String() << '\n';
    // std::cout << "STORING REMOTE CURSOR: " << client_id << " -> " << resolved->to_String() << '\n';
    remote_cursors.insert_or_assign(client_id, position);
    //remote_cursors[client_id] = position;
}

void EditorSession::receive_message(Message message) {
    incoming_queue.push(message);        
}

void EditorSession::flush_incoming() {
    while (!incoming_queue.empty()) {
        apply(incoming_queue.front());
        incoming_queue.pop();
    }
}

void EditorSession::apply_history(const std::vector<Operation>& history){
    for(const auto& op : history) {
        apply_operation(op, false);
    }
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
        }
        // case MessageType::CURSOR_UPDATE: { // at some point will switch to this; not yet
        //     const CursorUpdate& update = std::get<CursorUpdate>(message.get_payload());

        //     receive_cursor_update(message.get_sender(), update.position);
        //     break;
        // }
        default:
            break;
    }
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
}

void EditorSession::update_remote_cursors(const Operation& oper) {
    std::visit([&](const auto& operation) {
        using T = std::decay_t<decltype(operation)>;

        if constexpr (std::is_same_v<T, RemoveOperation>) {
            const ElementID& target = operation.get_target();

            std::cout << "REMOTE CURSOR DELETE UPDATE\n";
            std::cout << "  deleted target: " << target.to_String() << '\n';

            for (auto& [client_id, anchor] : remote_cursors) {
                std::cout << "  " << client_id << " cursor before: " << anchor.to_String() << '\n';
                
                if (anchor == target) {
                    std::cout << "before visible_predecessor in update_remote_cursors (anchor : target): " << anchor.to_String() << " : " << target.to_String() << std::endl;
                    ElementID vis_predecessor = doc.visible_predecessor(target);
                    std::cout << "  MOVING " << client_id << " -> " << vis_predecessor.to_String() << '\n';
                    anchor = vis_predecessor;
                }
            }
        }
    }, oper);
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


EditorSession::EditorSession(Document& d, Cursor& c, InputHandler& hand, OperationLog& l, Id_generator& g): doc(d), cursor(c), handler(hand), log(l), gen(g) {}