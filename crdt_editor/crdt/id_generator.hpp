#pragma once
#include <iostream>
#include <src/elementID.hpp>


class Id_generator {
public:

    explicit Id_generator(std::string c);

    ElementID next();

    void sync_clock(uint64_t remote_clock);

private:
    std::string client_ID;
    uint64_t lamport_clock = 0;
};