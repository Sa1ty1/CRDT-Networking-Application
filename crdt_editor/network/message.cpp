#include <network/message.hpp>




MessageType Message::get_type() const {return type;}
std::string Message::get_sender() const {return sender;}
const MessagePayload& Message::get_payload() const {return payload;}

std::string Message::serialize() const {
    std::string result;
    result += message_serializer::serialize_type(type);
    result += '\n';
    result += sender;
    result += '\n';
    std::visit([&](const auto& value) {
        using T = std::decay_t<decltype(value)>;

        if constexpr(std::is_same_v<T, std::monostate>) {
            return;
        } else if constexpr(std::is_same_v<T, Operation>) {
            result += operation_serializer::serialize(value);
        } else if constexpr(std::is_same_v<T, CursorUpdate>) {
            result += value.position.serialize();
        } else if constexpr(std::is_same_v<T, std::vector<Operation>>) {
            for (auto& op: value) {
                result += operation_serializer::serialize(op);
                result += '\n';
            }
        }
    }, payload);

    return result;
}

Message Message::deserialize(const std::string& serialized_message) {
    auto split_view = std::views::split(serialized_message, '\n');
    std::vector<std::string> tokens;

    for (auto&& subrange : split_view) { // the substrings; the middle two are the elementID
        tokens.emplace_back(subrange.begin(), subrange.end());
    }

    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << "TOKEN " << i << ": [" << tokens[i] << "]\n";
    }

    MessageType type = message_serializer::deserialize_type(tokens.at(0));
    std::string sender = tokens.at(1);
    switch (type) {
        case MessageType::HELLO: {
            return Message(type, sender, std::monostate{});
        }
        case MessageType::OPERATION: {
            if (tokens.size() < 3 || tokens[2].empty()) {
                throw std::invalid_argument("OPERATION message missing operation payload");
            }
            Operation op = operation_serializer::deserialize(tokens.at(2));
            return Message(type, sender, op);
        }
        case MessageType::SYNC_RESPONSE: {
            std::vector<Operation> ops;
            for (size_t i = 2; i < tokens.size(); i++) {
                if (tokens[i].empty()) {
                    continue;
                }
                ops.push_back(operation_serializer::deserialize(tokens.at(i)));
            }
            return Message(type, sender, ops);
        }
        case MessageType::SYNC_COMPLETE: {
            return Message(type, sender, std::monostate{});
        }
        case MessageType::CURSOR_UPDATE: {
            if (tokens.size() < 3 || tokens[2].empty()) {
                throw std::invalid_argument(
                    "CURSOR_UPDATE message missing cursor position"
                );
            }

            ElementID position =
                ElementID::deserialize(tokens[2]);

            return Message(
                type,
                sender,
                CursorUpdate{position}
            );
        }
        default: {
            throw std::invalid_argument("Unknown message type");
        }
    }

}

Message::Message(const MessageType& t, const std::string& s, const MessagePayload& mp) : type(std::move(t)), sender(std::move(s)), payload(std::move(mp)) {}

namespace message_serializer {
    std::string serialize_type(MessageType type) {
        switch (type) {
            case MessageType::HELLO:
                return "HELLO";
            case MessageType::OPERATION:
                return "OPERATION";
            case MessageType::SYNC_REQUEST:
                return "SYNC_REQUEST";
            case MessageType::SYNC_RESPONSE:
                return "SYNC_RESPONSE";
            case MessageType::SYNC_COMPLETE:
                return "SYNC_COMPLETE";
            case MessageType::CURSOR_UPDATE:
                return "CURSOR_UPDATE";
        }
        throw std::invalid_argument("Unknown message type");
    }
    MessageType deserialize_type(const std::string& s) {
        if (s == "HELLO") {
            return MessageType::HELLO;
        }
        if (s == "OPERATION") {
            return MessageType::OPERATION;
        } 
        if (s == "SYNC_REQUEST") {
            return MessageType::SYNC_REQUEST;
        }
        if (s == "SYNC_RESPONSE") {
            return MessageType::SYNC_RESPONSE;
        }
        if (s == "SYNC_COMPLETE") {
            return MessageType::SYNC_COMPLETE;
        }
        if (s == "CURSOR_UPDATE") {
            return MessageType::CURSOR_UPDATE;
        }
        throw std::invalid_argument("Unknown message type: " + s);
    }
}