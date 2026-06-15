#pragma once

#include <cassert>
#include <cstdint>
#include <concepts>
#include <stdexcept>
#include <limits>
#include <array>
#include <utility>

#include <print>

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
        return (index + attempt) & mask;
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
    typename    Probe = LinearProbe,
    bool        Rehash = false
>
class OpenAddressMap {

    static_assert(Capacity > 0 && "Capacity should not be 0");
    static_assert((Capacity & (Capacity - 1)) == 0 && "Capacity should be power of 2");

    static constexpr bool INSTRUMENT  = false;
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
    ~OpenAddressMap() {
        if constexpr (INSTRUMENT) {
            std::println("Operation Count: {}",  instrument_state_.operation_count);
            std::println("Rehash Count: {}",     instrument_state_.rehashes);
            std::println("Max Probe Length: {}", instrument_state_.max_probe_length);
            std::println("Max Tombstones: {}",   instrument_state_.max_tombstone);
            std::println("Max load factor: {}",  static_cast<double>(instrument_state_.max_size) / Capacity);
            std::println("Key Range: [{}, {}]",  instrument_state_.min_key, instrument_state_.max_key);
        }
    }

    OpenAddressMap(const OpenAddressMap&) = delete;
    void operator=(const OpenAddressMap&) = delete;

    OpenAddressMap(OpenAddressMap&&) = default;
    OpenAddressMap& operator=(OpenAddressMap&&) = default;

    Value& operator[](const Key& key) {
        auto& [current_key, value, state] = table_[probe_for_slot_idx(key)];

        if constexpr (INSTRUMENT) {
            ++instrument_state_.operation_count;
            if (state != SlotState::OCCUPIED) {
                state = SlotState::OCCUPIED;
                ++instrument_state_.size;
                instrument_state_.max_size = std::max(instrument_state_.max_size, instrument_state_.size);
            }

            instrument_state_.min_key = std::min(instrument_state_.min_key, key);
            instrument_state_.max_key = std::max(instrument_state_.max_key, key);
        } else {
            state = SlotState::OCCUPIED;
        }
        current_key = key;

        return value;
    }

    [[nodiscard]] Handle find(const Key& key) {
        if constexpr (INSTRUMENT) {
            instrument_state_.min_key = std::min(instrument_state_.min_key, key);
            instrument_state_.max_key = std::max(instrument_state_.max_key, key);
        }

        Slot& slot = table_[probe_for_slot_idx(key)];
        if (slot.state != SlotState::OCCUPIED || slot.key != key)
            return Handle{};

        return Handle{&slot};
    }

    void erase(Handle h) {
        h.slot_->state = SlotState::TOMBSTONE;     
        ++tombstone_;
        if constexpr (Rehash) {
            if (tombstone_ > (Capacity >> 2)) rehash();
        } 

        if constexpr (INSTRUMENT) {
            ++instrument_state_.operation_count;
            --instrument_state_.size;
            instrument_state_.max_tombstone = std::max(instrument_state_.max_tombstone, tombstone_);
        }
    }

    void erase(const Key& key) {
        if (auto h = find(key)) erase(h);
    }

private:

    struct InstrumentState {
        std::size_t size{};
        std::size_t rehashes{};
        std::size_t operation_count{};
        std::size_t max_probe_length{};
        std::size_t max_tombstone{};
        std::size_t max_size{};

        Key min_key_ = std::numeric_limits<Key>::max(), max_key_ = std::numeric_limits<Key>::min();
    };

    std::array<Slot, Capacity> table_;

    [[no_unique_address]]
    mutable std::size_t tombstone_{};
    [[no_unique_address]]
    mutable std::conditional_t<INSTRUMENT, InstrumentState, std::monostate> instrument_state_;

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

            if (slot.state == SlotState::TOMBSTONE && first_tombstone_idx == TOMBSTONE_SENTINEL) {
                first_tombstone_idx = probe_idx;
                --tombstone_;
            }
            
            probe_idx = Probe::next(probe_idx, ++attempt, MASK);
            
            if constexpr (INSTRUMENT) {
                instrument_state_.max_probe_length = std::max(instrument_state_.max_probe_length, attempt);
            }

            if (probe_idx == original_idx) [[unlikely]] {
                if (first_tombstone_idx == TOMBSTONE_SENTINEL)
                    throw MapSaturatedException{"Open Address Map full."};
                return first_tombstone_idx;
            }
        }

        std::unreachable();
    }

    void rehash() {
        tombstone_ = 0;
        if constexpr (INSTRUMENT) {
            ++instrument_state_.rehashes;
        }

        for (auto& slot: table_) {
            if (slot.state == SlotState::TOMBSTONE) slot.state = SlotState::EMPTY;
        }

        for (std::size_t i = 0; i < table_.size(); ++i) { 
            if (table_[i].state != SlotState::OCCUPIED) continue;
            auto evicted = table_[i];
            table_[i].state = SlotState::EMPTY;

            std::size_t idx = hash(evicted.key);
            std::size_t attempt = 0;
            while (table_[idx].state == SlotState::OCCUPIED) 
                idx = Probe::next(idx, ++attempt, MASK);
            table_[idx] = evicted;
        }
    }

};
