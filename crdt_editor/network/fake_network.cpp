#include <network/fake_network.hpp>


int FakeNetwork::get_default_delay() const {return default_delay;}
void FakeNetwork::set_default_delay(int delay) {default_delay = delay;}
int FakeNetwork::get_tick() const {return current_tick; }

void FakeNetwork::set_delivery_policy(DeliveryPolicy p) {
    current_policy = p;
}

void FakeNetwork::register_client(std::string clientID, EditorSession& sessionID) {
    client_session_pair.insert_or_assign(clientID, &sessionID);
}


void FakeNetwork::collect_outgoing() {
    for (const auto& [clientID, sessionID] : client_session_pair) {
        std::vector<Operation> operations = sessionID->take_outgoing_operations();
        for (int i = 0; i < operations.size(); i++) {
            for (const auto& [cID, sID] : client_session_pair) {
                if (cID != clientID) {
                    Message new_message = Message(MessageType::OPERATION, cID, operations.at(i));
                    std::string serialized_message = new_message.serialize();
                    in_flight.push_back(PendingMessage {serialized_message, choose_delivery_tick()});
                    //q.push(serialized_message);
                }
            }
        }
    }
}

int FakeNetwork::choose_delivery_tick() {
    std::uniform_int_distribution<int> delay(0,5);
    return current_tick + delay(rng);
}

void FakeNetwork::deliver(const std::string& sender, const std::string& mes) {
    Message message = Message::deserialize(mes);

    if (message.get_type() == MessageType::OPERATION) {

        std::visit([&](const auto& op) {
            using T = std::decay_t<decltype(op)>;
            if constexpr (std::is_same_v<T, InsertOperation>) {
                std::cout << "Delivering " << op.get_char() << std::endl;
            }
        }, std::get<Operation>(message.get_payload()));

        for (const auto& [client, session] : client_session_pair) {
            if (client != sender) {
                client_session_pair.at(client)->receive_message(message);
            }
        }
    }

}

int FakeNetwork::choose_ready_message(DeliveryPolicy policy) {
    switch (policy)
    {
    case DeliveryPolicy::FIFO: {
        for (int i = 0; i < in_flight.size(); i++) {
            if (in_flight.at(i).deliver_at_tick <= current_tick) {
                return i;
            }
        }   
        return -1; 
    }
    
    case DeliveryPolicy::LIFO: {
        for (int i = in_flight.size()-1; i >= 0; i--) {
            if (in_flight.at(i).deliver_at_tick <= current_tick) {
                return i;
            }
        }   
        return -1;
    }
    case DeliveryPolicy::SCRIPTED: {
        throw std::logic_error("SCRIPTED not implemented");
    }
    case DeliveryPolicy::RANDOM: {
        std::vector<int> options;
        for (int i = 0; i < in_flight.size(); i++) {
            if (in_flight.at(i).deliver_at_tick <= current_tick) {
                options.emplace_back(i);
            }
        }
        if (options.empty()) {
            return -1;
        }
        std::uniform_int_distribution<int> distrib(0, options.size() - 1);
        int random_num = distrib(rng);
        return options.at(random_num);
    }
    default: {
        throw(std::invalid_argument("This is not a valid policy"));
    }
    }
}

void FakeNetwork::deliver_next_ready() {
    while (true) {
        int index = choose_ready_message(current_policy);
        if (index == -1) {
            break;
        }
        std::string temp = Message::deserialize(in_flight.at(index).serialized_message).get_sender();
        deliver(temp, in_flight.at(index).serialized_message);
        in_flight.erase(in_flight.begin() + index);
    }
}

int FakeNetwork::pending_message() {
    return in_flight.size();
}

void FakeNetwork::tick() {
    collect_outgoing();
    deliver_next_ready();
    current_tick++;
}

FakeNetwork::FakeNetwork(uint32_t seed) : rng(seed) {}