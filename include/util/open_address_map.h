#pragma once

#include <cassert>
#include <cstdint>
#include <concepts>
#include <stdexcept>
#include <limits>
#include <array>
#include <utility>

template<typename T>
concept Probe = requires (std::size_t index, std::size_t attempt, std::size_t mask) {
    { T::next(index, attempt, mask) } -> std::same_as<std::size_t>;
};

struct LinearProbe {
    static constexpr std::size_t next(std::size_t index, std::size_t /* attempt */, std::size_t mask) {
        assert((mask & (mask + 1)) == 0 && "Invalid mask: mask + 1 should be power of 2");
        return (index + 1) & mask;
    }
};

struct QuadraticProbe {
    static constexpr std::size_t next(std::size_t index, std::size_t attempt, std::size_t mask) {
        assert((mask & (mask + 1)) == 0 && "Invalid mask: mask + 1 should be power of 2");
        return (index + attempt * attempt) & mask;
    }
};

class MapSaturatedException : public std::runtime_error {
public:
    explicit MapSaturatedException(const std::string& message)
        : std::runtime_error(message) {}
};

template<
    std::size_t Capacity,
    typename    Key,   // TODO: make safe to cast to size_t
    typename    Value,
    typename    Probe = LinearProbe
>
class OpenAddressMap {

    static_assert(Capacity > 0 && "Capacity should not be 0");
    static_assert((Capacity & (Capacity - 1)) == 0 && "Capacity should be power of 2");

    static constexpr std::size_t MASK = Capacity - 1; 

    enum class SlotState: std::uint8_t { EMPTY, OCCUPIED, TOMBSTONE };

    struct Slot {
        Key       key{};
        Value     value{};
        SlotState state = SlotState::EMPTY;
    };

public:

    class Handle {
    private:
        friend class OpenAddressMap;
        Slot* slot_ = nullptr;
        explicit Handle(Slot* slot) : slot_(slot) {}
    public:
        Handle() = default;
        explicit operator bool() { return slot_ != nullptr; }
        Value& operator*()  { return slot_->value; }
        Value* operator->() { return &slot_->value; }
    };

    OpenAddressMap() = default;
    ~OpenAddressMap() = default;

    OpenAddressMap(const OpenAddressMap&) = delete;
    void operator=(const OpenAddressMap&) = delete;

    OpenAddressMap(OpenAddressMap&&) = default;
    OpenAddressMap& operator=(OpenAddressMap&&) = default;

    Value& operator[](const Key& key) {
        auto& [current_key, value, state] = table_[probe_for_slot_idx(key)];
        state = SlotState::OCCUPIED;
        current_key = key;
        return value;
    }

    [[nodiscard]] Handle find(const Key& key) {
        Slot& slot = table_[probe_for_slot_idx(key)];
        if (slot.state != SlotState::OCCUPIED || slot.key != key)
            return Handle{};

        return Handle{&slot};
    }

    void erase(Handle h) {
        h.slot_->state = SlotState::TOMBSTONE;     
    }

    void erase(const Key& key) {
        if (auto h = find(key)) erase(h);
    }

private:

    std::array<Slot, Capacity> table_;

    [[nodiscard]] std::size_t hash(const Key& key) const {
        return key & MASK;
    }

    [[nodiscard]] std::size_t probe_for_slot_idx(const Key& key) const {
        std::size_t original_idx = hash(key);
        std::size_t probe_idx = original_idx;
        
        std::size_t attempt = 0;

        static constexpr auto TOMBSTONE_SENTINEL = std::numeric_limits<std::size_t>::max();
        std::size_t first_tombstone_idx = TOMBSTONE_SENTINEL;

        while (true) {
            const Slot& slot = table_[probe_idx];

            if (slot.state == SlotState::OCCUPIED && slot.key == key) return probe_idx;

            if (slot.state == SlotState::EMPTY) 
                return first_tombstone_idx != TOMBSTONE_SENTINEL 
                    ? first_tombstone_idx
                    : probe_idx;

            if (slot.state == SlotState::TOMBSTONE && first_tombstone_idx == TOMBSTONE_SENTINEL)
                first_tombstone_idx = probe_idx;
            
            probe_idx = Probe::next(probe_idx, ++attempt, MASK);

            if (probe_idx == original_idx) [[unlikely]] {
                if (first_tombstone_idx == TOMBSTONE_SENTINEL)
                    throw MapSaturatedException{"Open Address Map full."};
                return first_tombstone_idx;
            }
        }

        std::unreachable();
    }

};
