#include <crdt/id_generator.hpp>


Id_generator::Id_generator(std::string c) : client_ID(std::move(c)) {}

ElementID Id_generator::next() {
    return ElementID(++lamport_clock, client_ID);
}

void Id_generator::sync_clock(uint64_t remote_clock) {
    lamport_clock = std::max(lamport_clock, remote_clock);
}
