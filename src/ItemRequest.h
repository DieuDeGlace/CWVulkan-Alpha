#pragma once

#include "creatures/Inventory.h"
#include <cstring>

// Packet 6 and the offline actor queue carry the same interaction fields.
struct ItemRequest {
    enum Kind : uint8_t {
        UseItem = 1, Talk = 2, UseStatic = 3, PickUp = 5, Drop = 6, PlaceObject = 7, CallPet = 8
    };
    using Packet = std::array<uint8_t, 300>;

    Kind kind;
    creatures::ItemRecord item{};
    std::array<int32_t, 3> target{-1, -1, -1};

    Packet packet() const {
        Packet bytes{};
        std::memcpy(bytes.data(), item.data(), item.size());
        std::memcpy(bytes.data() + 0x118, target.data(), 12);
        bytes[0x128] = kind;
        bytes[0x12a] = 1;
        return bytes;
    }

    static ItemRequest parse(const Packet &bytes) {
        ItemRequest request{static_cast<Kind>(bytes[0x128])};
        std::memcpy(request.item.data(), bytes.data(), request.item.size());
        std::memcpy(request.target.data(), bytes.data() + 0x118, 12);
        return request;
    }
};
