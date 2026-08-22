#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <variant>
#include <random>
#include <unordered_map>
#include <network/message.hpp>
#include <network/editor_session.hpp>


enum class DeliveryPolicy {
    FIFO,
    LIFO,
    SCRIPTED,
    RANDOM
};

class FakeNetwork {

    struct PendingMessage {
            std::string serialized_message;
            int deliver_at_tick;
    };

public:

    int get_default_delay() const;
    void set_default_delay(int delay);
    int get_tick() const;

    void set_delivery_policy(DeliveryPolicy p);

    void register_client(std::string clientID, EditorSession& sessionID);


    void collect_outgoing();

    int choose_delivery_tick();

    void deliver(const std::string& sender, const std::string& mes);

    int choose_ready_message(DeliveryPolicy policy);

    void deliver_next_ready();

    int pending_message();

    void tick();

    FakeNetwork(uint32_t seed);
private:
    std::mt19937 rng;
    std::unordered_map<std::string, EditorSession*> client_session_pair;
    int current_tick = 0;
    int default_delay = 3;
    DeliveryPolicy current_policy = DeliveryPolicy::FIFO;
    std::vector<PendingMessage> in_flight;    
};
