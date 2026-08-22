#pragma once
#include <string>
#include <ranges>
#include <optional>
#include<src/operation.hpp>

struct CursorUpdate {
    ElementID position;
};

enum class MessageType {
    HELLO,
    OPERATION,
    SYNC_REQUEST,
    SYNC_RESPONSE,
    SYNC_COMPLETE,
    CURSOR_UPDATE,
};

using MessagePayload = std::variant<std::monostate, Operation, std::vector<Operation>, CursorUpdate>;

std::string serialize_message_type(MessageType type);
MessageType deserialize_message_type(const std::string& s);

class Message {
public:

    MessageType get_type() const;

    std::string get_sender() const;

    const MessagePayload& get_payload() const;

    std::string serialize() const;

    static Message deserialize(const std::string& serialized_message);

    Message(const MessageType& type, const std::string& s, const MessagePayload& payload);

private:
    MessageType type;
    std::string sender;
    //std::optional<Operation> operation; // not Operation& because we want a self-contained copy
    MessagePayload payload;
    // at some point
    // timestamp
    // sequence number
    // message_id
};

namespace message_serializer {
    std::string serialize_type(MessageType type);

    MessageType deserialize_type(const std::string& s);
}
