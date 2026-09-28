#ifndef BIZIUTILS_H
#define BIZIUTILS_H

#include "dealer.h"

// T CardStateArray[52], store any type per card, locate it by KCard* or (rank, suit)
template<typename T>
class CardStateArray
{
private:
    static constexpr size_t NUM_CARDS = 52;

    std::array<T, NUM_CARDS> m_registry{};

    inline size_t get_index(int rank, int suit) const
    {
        return (static_cast<size_t>(suit) * 13) + (static_cast<size_t>(rank) - 1);
    }

    inline size_t get_index(const KCard *card) const
    {
        return get_index(card->rank(), card->suit());
    }

public:
    CardStateArray()
    {
        reset();
    }

    void set(int rank, int suit, T state)
    {
        assert(rank >= 1 && rank <= 13 && suit >= 0 && suit < 4);
        m_registry[get_index(rank, suit)] = state;
    }

    void set(const KCard *card, T state)
    {
        if (card) {
            m_registry[get_index(card)] = state;
        }
    }

    T get(int rank, int suit) const
    {
        return m_registry[get_index(rank, suit)];
    }

    T get(const KCard *card) const
    {
        if (!card)
            return nullptr;
        return m_registry[get_index(card)];
    }

    void reset()
    {
        m_registry.fill(0);
    }
};

// CardStateMap, a map of any cards to any type T, store whatever you need, lookup by KCard* or (rank, suit)
// Rough example:
//  CardStateMap<uint8_t> chain_leader;
//  chain_leader.set(KCardDeck::Jack, KCardDeck::Clubs, 1); // Note Jack o Clubs as chain leader

template<typename T>
class CardStateMap
{
private:
    std::unordered_map<uint8_t, T> m_registry;

    inline uint8_t get_key(int rank, int suit) const
    {
        return (suit << 4) | (rank & 0x0F);
    }

    inline uint8_t get_key(const KCard *card) const
    {
        return get_key(card->rank(), card->suit());
    }

public:
    void set(int rank, int suit, T state)
    {
        m_registry[get_key(rank, suit)] = std::move(state);
    }

    void set(const KCard *card, T state)
    {
        if (card)
            m_registry[get_key(card)] = std::move(state);
    }

    std::optional<T> get(int rank, int suit) const
    {
        auto it = m_registry.find(get_key(rank, suit));
        if (it != m_registry.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::optional<T> get(const KCard *card) const
    {
        if (!card)
            return std::nullopt;
        return get(card->rank(), card->suit());
    }

    void reset()
    {
        m_registry.clear();
    }
};

#endif // BIZIUTILS_H
