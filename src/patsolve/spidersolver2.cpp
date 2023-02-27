/* Copyright (C) 2023 Stephan Kulow <coolo@kde.org>
   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "spidersolver2.h"
#include "../spider.h"
#include <QDebug>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
// #include <time.h>
#include <unordered_map>
#include <unordered_set>

namespace spidersolver2
{
static volatile bool stop_exec = false;

enum Suit { Clubs = 0, Diamonds, Hearts, Spades };
enum Rank { None = 0, Ace = 1, Two = 2, Three = 3, Four = 4, Five = 5, Six = 6, Seven = 7, Eight = 8, Nine = 9, Ten = 10, Jack = 11, Queen = 12, King = 13 };

// give magic numbers a name
const int REDEALS_NR = 6;
const int MAX_MOVES = 230;

struct Card {
    // 4 bits rank
    // 2 bits suit
    // 1 bit faceup
    // 1 bit unknown
    unsigned char value;

    Card()
    {
        value = 0;
    }

    Card(unsigned char v)
    {
        value = v;
    }

    Card(KCard *c)
    {
        value = 0;
        set_faceup(c->isFaceUp());
        set_rank(Rank(c->rank()));
        set_suit(Suit(c->suit()));
    }

    inline bool is_faceup() const
    {
        return (value & (1 << 6)) > 0;
    }

    void set_faceup(bool face)
    {
        if (face) {
            value = value | (1 << 6);
        } else {
            value = value & ~(1 << 6);
        }
    }

    inline bool is_unknown() const
    {
        return (value & (1 << 7)) > 0;
    }

    void set_unknown(bool unknown)
    {
        if (unknown) {
            value = value | (1 << 7);
        } else {
            value = value & ~(1 << 7);
        }
    }

    inline Rank rank() const
    {
        return Rank(value & 15);
    }

    void set_rank(Rank rank)
    {
        value = (value & ~15) + rank;
    }

    inline Suit suit() const
    {
        return Suit((value >> 4) & 3);
    }

    void set_suit(Suit suit)
    {
        Rank _rank = rank();
        value = value >> 4;
        value = (value & ~3) + suit;
        value = (value << 4) + _rank;
    }

    bool inSequenceTo(const Card &other) const;
    std::string toString() const;
    unsigned char raw_value() const
    {
        return value;
    }
    bool operator==(const Card &rhs) const;
};

inline bool Card::inSequenceTo(const Card &other) const
{
    return other.is_faceup() && other.suit() == suit() && other.rank() == rank() + 1;
}

std::string Card::toString() const
{
    if (is_unknown() && is_faceup())
        return "XX";
    if (is_unknown())
        return "|XX";

    std::string ret;
    switch (rank()) {
    case Ace:
        ret += 'A';
        break;
    case Jack:
        ret += 'J';
        break;
    case Queen:
        ret += 'Q';
        break;
    case King:
        ret += 'K';
        break;
    case Ten:
        ret += 'T';
        break;
    case None:
        ret += 'N';
        break;
    default:
        if (rank() < 2 || rank() > 9) {
            printf("rank is out of range\n");
            exit(1);
        }
        ret += ('0' + rank());
        break;
    }
    switch (suit()) {
    case Spades:
        ret += "S";
        break;
    case Hearts:
        ret += "H";
        break;
    case Diamonds:
        ret += "D";
        break;
    case Clubs:
        ret += "C";
        break;
    default:
        std::cerr << "Invalid suit " << suit() << std::endl;
        exit(1);
    }
    if (!is_faceup())
        ret = "|" + ret;
    return ret;
}

// to remove known cards from partly unknown decks. We don't care for faceup and unknown
bool Card::operator==(const Card &rhs) const
{
    return suit() == rhs.suit() && rank() == rhs.rank();
}

struct Move {
    bool off;
    bool talon;
    unsigned char from;
    unsigned char to;
    unsigned char index;
    Move()
    {
        talon = false;
        off = false;
        from = -1;
        to = -1;
        index = 0;
    }
    Move(bool _off, bool _talon, int _from, int _to, int _index)
    {
        off = _off;
        talon = _talon;
        from = _from;
        to = _to;
        index = _index;
    }
    static Move fromTalon(int talon)
    {
        return Move(false, true, talon, 0, 0);
    }
    static Move toOff(int from, int index)
    {
        return Move(true, false, from, 0, index);
    }
    static Move regular(int from, int to, int index)
    {
        return Move(false, false, from, to, index);
    }
    MOVE to_MOVE() const;
};

MOVE Move::to_MOVE() const
{
    MOVE ret;
    ret.card_index = index;
    ret.from = from;
    if (talon) {
        ret.from = 11;
    }
    ret.to = to;
    if (off) {
        ret.totype = O_Type;
        ret.pri = 127;
    } else {
        ret.totype = W_Type;
    }
    return ret;
}

const int MAX_CARDS = 104;

class Pile;
class Pile
{
    friend class MemoryManager;

public:
    Pile()
    {
        count = 0;
    }
    const Pile *addCard(const Card &c) const;
    std::string toString() const;
    bool empty() const
    {
        return count == 0;
    }
    const Card at(int index) const
    {
        return Card(cards[index]);
    }

    inline size_t cardCount() const
    {
        return count;
    }
    const Pile *remove(int index) const;
    const Pile *copyFrom(const Pile *from, int index) const;
    const Pile *replaceAt(int index, const Card &c) const;
    int chaos() const
    {
        return m_chaos;
    }
    void calculateChaos();
    int sequenceOf(Suit suit) const
    {
        return m_seqs[suit];
    }
    int playableCards() const;
    uint64_t hash() const
    {
        return m_hash;
    }

private:
    int m_chaos;
    unsigned char cards[MAX_CARDS];
    size_t count;
    uint64_t m_hash;
    int m_seqs[4];
    int sequenceOf_(Suit suit) const;
    void setAt(int index, const Card &c)
    {
        cards[index] = c.raw_value();
    }
};

class Deck
{
private:
    // Deck(const Deck &other);

public:
    Deck()
    {
        // leaving the pointers stale on purpose
        // for performance
    }

    std::string toString() const;
    void update(const Deck *);
    void translate(const Spider *dealer);
    void getMoves(std::vector<Move> &moves) const;
    void applyMove(const Move &m, Deck &newdeck) const;
    uint64_t id() const;
    int leftTalons() const;
    int chaos() const;
    SolverInterface::ExitStatus shortestPath();
    int playableCards() const;
    int inOff() const;
    int freePlays() const;
    void makeEmpty();
    std::vector<Card> parse(int game_type, const std::string &filename);
    std::vector<Move> getWinMoves() const;

private:
    const Pile *play[10];
    const Pile *talon[5];
    const Pile *off;
    Move moves[MAX_MOVES];
    int moves_index;
};

class MemoryManager
{
public:
    MemoryManager();
    ~MemoryManager();
    const Pile *createEmptyPile();
    const Pile *translatePile(const KCardPile *pile, bool reverse);
    const Pile *createPile(const unsigned char *cards, size_t count);
    Deck &deckAt(size_t index)
    {
        return deckstore[index];
    }
    void clean();
    static MemoryManager *self()
    {
        return m_self;
    }

private:
    static MemoryManager *m_self;
    std::unordered_map<uint64_t, Pile *> pilemap;
    Deck *deckstore;
};

const Pile *MemoryManager::createPile(const unsigned char *cards, size_t count)
{
    std::hash<unsigned char> hasher;
    std::size_t seed = 0;
    for (size_t i = 0; i < count; i++) {
        seed ^= hasher(cards[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    auto search = pilemap.find(seed);
    if (search != pilemap.end())
        return search->second;
    Pile *p = new Pile;
    memcpy(p->cards, cards, count);
    p->count = count;
    p->calculateChaos();
    p->m_hash = seed;
    p->m_seqs[Hearts] = p->sequenceOf_(Hearts);
    p->m_seqs[Spades] = p->sequenceOf_(Spades);
    p->m_seqs[Clubs] = p->sequenceOf_(Clubs);
    p->m_seqs[Diamonds] = p->sequenceOf_(Diamonds);
    pilemap.insert({seed, p});
    return p;
}

std::string Pile::toString() const
{
    std::string ret;
    for (size_t i = 0; i < count; i++) {
        ret += " " + Card(cards[i]).toString();
    }
    return ret;
}

const Pile *Pile::remove(int index) const
{
    if (index > 0) {
        Card c = at(index - 1);
        if (!c.is_faceup()) {
            static unsigned char newcards[MAX_CARDS];
            memcpy(newcards, cards, MAX_CARDS);
            c.set_faceup(true);
            newcards[index - 1] = c.raw_value();
            return MemoryManager::self()->createPile(newcards, index);
        } else {
            return MemoryManager::self()->createPile(cards, index);
        }
    } else {
        return MemoryManager::self()->createPile(cards, 0);
    }
}

const Pile *Pile::copyFrom(const Pile *from, int index) const
{
    unsigned char newcards[MAX_CARDS];
    memcpy(newcards, cards, count);
    size_t newcount = count;
    for (size_t i = index; i < from->count; i++) {
        newcards[newcount++] = from->cards[i];
    }
    return MemoryManager::self()->createPile(newcards, newcount);
}

const Pile *Pile::addCard(const Card &c) const
{
    unsigned char newcards[MAX_CARDS];
    memcpy(newcards, cards, MAX_CARDS);
    newcards[count] = c.raw_value();
    return MemoryManager::self()->createPile(newcards, count + 1);
}

void Pile::calculateChaos()
{
    m_chaos = 0;
    Card lastcard;
    for (size_t i = 0; i < cardCount(); i++) {
        Card current = at(i);

        // first in stack
        if (lastcard.raw_value() == 0) {
            m_chaos++;
        } else {
            if (!current.inSequenceTo(lastcard)) {
                m_chaos++;
            }
        }
        lastcard = current;
    }
}

const Pile *Pile::replaceAt(int index, const Card &c) const
{
    unsigned char newcards[MAX_CARDS];
    memcpy(newcards, cards, MAX_CARDS);
    newcards[index] = c.raw_value();
    return MemoryManager::self()->createPile(newcards, count);
}

int Pile::sequenceOf_(Suit suit) const
{
    int index = cardCount();
    if (index == 0) {
        return index;
    }
    index--;
    Card top_card = at(index);
    if (top_card.suit() != suit) {
        return 0;
    }
    while (index > 0 && top_card.inSequenceTo(at(index - 1))) {
        index--;
        top_card = at(index);
    }
    return cardCount() - index;
}

int Pile::playableCards() const
{
    if (count < 2) {
        return count;
    }
    return sequenceOf(at(count - 1).suit());
}

struct WeightedDeck {
    unsigned char left_talons;
    unsigned char in_off;
    unsigned char free_plays;
    unsigned char playable;
    int32_t chaos;
    int64_t id;
    int32_t index;

    bool operator<(const WeightedDeck &rhs) const;
    void update(uint64_t hash);
};

void WeightedDeck::update(uint64_t hash)
{
    Deck &deck = MemoryManager::self()->deckAt(index);
    left_talons = deck.leftTalons();
    id = hash;
    in_off = deck.inOff();
    free_plays = deck.freePlays();
    playable = deck.playableCards();
    chaos = deck.chaos();
}

// smaller is better!
bool WeightedDeck::operator<(const WeightedDeck &rhs) const
{
    if (chaos != rhs.chaos) {
        // smaller chaos is better
        return chaos < rhs.chaos;
    }
    int ready1 = playable + in_off + free_plays;
    int ready2 = rhs.playable + rhs.in_off + rhs.free_plays;
    if (ready1 != ready2) {
        // larger values are better
        return ready1 > ready2;
    }

    // once we are in straight win mode, we go differently
    if (chaos == 0) {
        int free1 = free_plays;
        int free2 = rhs.free_plays;

        if (free1 != free2) {
            // more free is better
            return free1 > free2;
        }
        // if the number of empty plays is equal, less in the off
        // is actually a benefit (more strongly ordered)
        int off1 = in_off;
        int off2 = rhs.in_off;
        if (off1 != off2) {
            return off1 < off2;
        }
    }
    // give a reproducible sort order, but std::sort doesn't give
    // guarantess for equal items, so prefer them being different
    return id < rhs.id;
}

std::string Deck::toString() const
{
    std::string ret;
    char buffer[200];
    int counter = 0;
    for (int i = 0; i < 10; i++) {
        snprintf(buffer, sizeof(buffer), "Play%d:", i);
        ret += buffer;
        ret += play[i]->toString();
        ret += "\n";
    }

    for (int i = 0; i < 5; i++) {
        snprintf(buffer, sizeof(buffer), "Deal%d:", i);
        ret += buffer;

        ret += talon[i]->toString();
        ret += "\n";
        counter++;
    }

    ret += "Off:";
    ret += off->toString();
    ret += "\n";

    return ret;
}

void Deck::getMoves(std::vector<Move> &moves) const
{
    moves.clear();
    if (moves_index >= MAX_MOVES - 1) {
        return;
    }

    int next_talon = -1;
    for (int i = 0; i < 5; i++) {
        if (!talon[i]->empty()) {
            next_talon = i;
            break;
        }
    }

    int from = 0;
    bool one_is_empty = false;
    for (; from < 10; from++) {
        if (play[from]->empty()) {
            one_is_empty = true;
            continue;
        }

        int index = play[from]->cardCount() - 1;
        Suit top_suit = play[from]->at(index).suit();
        int top_rank = int(play[from]->at(index).rank()) - 1;

        while (index >= 0) {
            Card current = play[from]->at(index);
            if (!current.is_faceup())
                break;
            if (current.suit() != top_suit)
                break;
            if (top_rank + 1 != current.rank())
                break;
            top_rank = current.rank();

            if (play[from]->cardCount() - index == 13) {
                moves.clear();
                moves.push_back(Move::toOff(from, index));
                return;
            }
            int broken_sequence = 0;
            if (index > 0) {
                Card next_card = play[from]->at(index - 1);
                if (current.inSequenceTo(next_card)) {
                    broken_sequence = play[from]->cardCount() - index;
                }
            }
            bool moved_to_empty = false;
            for (int to = 0; to < 10; to++) {
                if (to == from)
                    continue;
                int to_count = play[to]->cardCount();
                if (to_count > 0) {
                    Card top_card = play[to]->at(to_count - 1);
                    if (top_card.rank() != top_rank + 1)
                        continue;
                    // make sure we only enlarge sequences
                    if (broken_sequence > 0) {
                        if (play[to]->sequenceOf(top_suit) + broken_sequence <= play[from]->sequenceOf(top_suit)) {
                            continue;
                        }
                    }
                } else if (moved_to_empty) {
                    if (next_talon < 0)
                        continue;
                } else {
                    // while talons are there, optimisations are evil
                    // but in end game we have more options
                    if (next_talon < 0) {
                        if (index == 0) {
                            // forbid moves between empty cells once the talons are gone
                            continue;
                        }
                        // there is no plausible reason to split up sequences in end game
                        if (broken_sequence > 0) {
                            continue;
                        }
                    }
                    moved_to_empty = true;
                }

                moves.push_back(Move::regular(from, to, index));
            }
            index--;
        }
    }

    if (!one_is_empty && next_talon >= 0) {
        moves.push_back(Move::fromTalon(next_talon));
    }
}

void Deck::update(const Deck *other)
{
    if (this == other)
        return;
    memcpy(moves, other->moves, sizeof(Move) * other->moves_index);
    moves_index = other->moves_index;
    for (int i = 0; i < 10; i++)
        play[i] = other->play[i];
    for (int i = 0; i < 5; i++)
        talon[i] = other->talon[i];
    off = other->off;
}

std::vector<Move> Deck::getWinMoves() const
{
    std::vector<Move> res;
    for (int i = 0; i < moves_index; i++) {
        res.emplace_back(moves[i]);
    }
    return res;
}

void Deck::applyMove(const Move &m, Deck &newdeck) const
{
    newdeck.update(this);
    newdeck.moves[moves_index] = m;
    newdeck.moves_index = moves_index + 1;

    if (m.talon) {
        for (int to = 0; to < 10; to++) {
            Card c = newdeck.talon[m.from]->at(to);
            c.set_faceup(true);
            newdeck.play[to] = newdeck.play[to]->addCard(c);
        }
        // empty pile
        newdeck.talon[m.from] = MemoryManager::self()->createEmptyPile();
    } else if (m.off) {
        Card c = newdeck.play[m.from]->at(newdeck.play[m.from]->cardCount() - 13);
        newdeck.off = newdeck.off->addCard(c);
        newdeck.play[m.from] = newdeck.play[m.from]->remove(m.index);
    } else {
        newdeck.play[m.to] = newdeck.play[m.to]->copyFrom(newdeck.play[m.from], m.index);
        newdeck.play[m.from] = newdeck.play[m.from]->remove(m.index);
    }
}

template<class T>
inline void hash_combine(std::size_t &seed, const T &v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

uint64_t Deck::id() const
{
    std::size_t hash = 0;
    for (int i = 0; i < 10; i++) {
        hash_combine(hash, play[i]->hash());
    }
    for (int i = 0; i < 5; i++) {
        hash_combine(hash, talon[i]->hash());
    }
    return hash;
}

int Deck::leftTalons() const
{
    int talons = 0;
    for (int i = 0; i < 5; i++) {
        if (!talon[i]->empty()) {
            talons++;
        }
    }
    return talons;
}

int Deck::chaos() const
{
    int chaos = 0;
    for (int i = 0; i < 10; i++) {
        chaos += play[i]->chaos();
    }
    // per non-empty pile the chaos is at minimum 1
    // but if the pile is connected, we substract one
    // obvious wins are chaos 0
    for (int i = 0; i < 10; i++) {
        if (play[i]->cardCount() == 0) {
            continue;
        }
        Card c1 = play[i]->at(0);

        if (c1.rank() == 13) {
            chaos -= 1;
            continue;
        }

        for (int j = 0; j < 10; j++) {
            if (j == i) {
                continue;
            }
            if (play[j]->empty()) {
                continue;
            }
            // we don't need the suit here
            if (c1.rank() == play[j]->at(play[j]->cardCount() - 1).rank() - 1) {
                chaos--;
                break;
            }
        }
    }
    int fp = freePlays();
    while (fp > 0 && chaos > 0) {
        fp--;
        chaos--;
    }
    return chaos;
}

SolverInterface::ExitStatus Deck::shortestPath()
{
    int depth = 1;
    bool discarded_siblings = false;

    Deck *unvisited = new Deck[REDEALS_NR * LEVEL_CAP];
    int unvisited_count[6] = {0, 0, 0, 0, 0, 0};
    int unvisited_count_total = 0;
    unvisited[unvisited_count_total++].update(this);

    std::unordered_set<uint64_t> seen;
    const int max_new_unvisited = LEVEL_CAP * REDEALS_NR * AVG_OPTIONS;
    WeightedDeck *new_unvisited = new WeightedDeck[max_new_unvisited];
    for (int i = 0; i < max_new_unvisited; i++) {
        new_unvisited[i].index = i;
    }
    int new_unvisited_counter = 0;
    std::vector<Move> current_moves;
    while (!stop_exec) {
        for (int i = 0; i < unvisited_count_total; i++) {
            unvisited[i].getMoves(current_moves);
            for (const Move &m : current_moves) {
                size_t deck_index = new_unvisited[new_unvisited_counter].index;
                Deck &deck = MemoryManager::self()->deckAt(deck_index);
                unvisited[i].applyMove(m, deck);
                uint64_t hash = deck.id();

                if (seen.find(hash) != seen.end())
                    continue;

                new_unvisited[new_unvisited_counter++].update(hash);
                seen.insert(hash);
                if (max_new_unvisited == new_unvisited_counter) {
                    std::cerr << "Too many unvisted " << new_unvisited_counter << " AVG too small" << std::endl;
                    return SolverInterface::MemoryLimitReached;
                }
            }
        }
        for (int lt = 0; lt <= 5; lt++)
            unvisited_count[lt] = 0;

        unvisited_count_total = 0;

        if (new_unvisited_counter == 0)
            break;

        bool printed = false;
        std::sort(new_unvisited, new_unvisited + new_unvisited_counter);
        for (int i = 0; i < new_unvisited_counter; i++) {
            if (!printed) {
#if 0
                std::cout << "DEPTH " << depth << " " << new_unvisited_counter << " chaos: " << new_unvisited[i].chaos << " " << int(new_unvisited[i].playable)
                          << std::endl;
#endif
                printed = true;
            }
            if (new_unvisited[i].in_off == 104) {
                Deck &deck = MemoryManager::self()->deckAt(new_unvisited[i].index);
                memcpy(moves, deck.moves, sizeof(Move) * deck.moves_index);
                moves_index = deck.moves_index;

                delete[] unvisited;
                delete[] new_unvisited;
                return SolverInterface::SolutionExists;
            }
            int lt = new_unvisited[i].left_talons;
            if (unvisited_count[lt] < LEVEL_CAP) {
                Deck &deck = MemoryManager::self()->deckAt(new_unvisited[i].index);
                unvisited[unvisited_count_total++].update(&deck);
                unvisited_count[lt]++;
            } else {
                discarded_siblings = true;
            }
        }

        new_unvisited_counter = 0;
        depth += 1;
    }
    delete[] unvisited;
    delete[] new_unvisited;
    if (new_unvisited_counter == 0 && !discarded_siblings) {
        return SolverInterface::NoSolutionExists;
    }
    return SolverInterface::UnableToDetermineSolvability;
}

int Deck::playableCards() const
{
    int result = 0;
    for (int i = 0; i < 10; i++)
        result += play[i]->playableCards();
    return result;
}

int Deck::inOff() const
{
    return off->cardCount() * 13;
}

int Deck::freePlays() const
{
    int result = 0;
    for (int i = 0; i < 10; i++) {
        if (play[i]->empty()) {
            result++;
        }
    }
    return result;
}

void Deck::makeEmpty()
{
    off = MemoryManager::self()->createEmptyPile();
    for (int i = 0; i < 10; i++)
        play[i] = MemoryManager::self()->createEmptyPile();
    for (int i = 0; i < 5; i++)
        talon[i] = MemoryManager::self()->createEmptyPile();
    moves_index = 0;
}

void Deck::translate(const Spider *dealer)
{
    makeEmpty();
    for (int i = 0; i < 5; i++) {
        talon[i] = MemoryManager::self()->translatePile(dealer->getRedeal(i), true);
    }
    for (int i = 0; i < 10; i++) {
        play[i] = MemoryManager::self()->translatePile(dealer->getStack(i), false);
    }
    for (int i = 0; i < 8; i++) {
        if (dealer->getLeg(i)->isEmpty())
            continue;
        off = off->addCard(Card(dealer->getLeg(i)->topCard()));
    }
    // std::cout << toString();
}

SpiderSolver2::SpiderSolver2(Spider *dealer)
{
    m_dealer = dealer;
    orig = new Deck();
    // singleton
    memManager = new MemoryManager();
}

SpiderSolver2::~SpiderSolver2()
{
    delete orig;
    delete memManager;
}

SolverInterface::ExitStatus SpiderSolver2::patsolve(int max_positions)
{
    m_winMoves.clear();
    if (max_positions > 0) {
        // rely on brute force hints - they are good enough
        return UnableToDetermineSolvability;
    }
    stop_exec = false;

    ExitStatus ret = orig->shortestPath();
    if (ret == SolutionExists) {
        for (auto &move : orig->getWinMoves())
            m_winMoves.push_back(move.to_MOVE());
    }
    return ret;
}

void SpiderSolver2::translate_layout()
{
    MemoryManager::self()->clean();
    orig->translate(m_dealer);
}

MoveHint SpiderSolver2::translateMove(const MOVE &m)
{
    const PatPile *frompile = nullptr;
    if (m.from < 10)
        frompile = m_dealer->getStack(m.from);
    else
        return MoveHint();

    KCard *card = frompile->at(m.card_index);
    Q_ASSERT(card);
    Q_ASSERT(m.to < 10);
    if (m.totype == W_Type)
        return MoveHint(card, m_dealer->getStack(m.to), m.pri);

    // auto move to the off
    return MoveHint();
}

void SpiderSolver2::stopExecution()
{
    stop_exec = true;
}

MemoryManager *MemoryManager::m_self = nullptr;
MemoryManager::MemoryManager()
{
    Q_ASSERT(!m_self);
    m_self = this;
    size_t deckstore_size = LEVEL_CAP * REDEALS_NR * AVG_OPTIONS;
    // do not call constructors!!
    deckstore = (Deck *)malloc(sizeof(Deck) * deckstore_size);
}

void MemoryManager::clean()
{
    for (auto &it : pilemap) {
        delete it.second;
    }
    pilemap.clear();
}

MemoryManager::~MemoryManager()
{
    clean();
    free(deckstore);
    m_self = nullptr;
}

const Pile *MemoryManager::createEmptyPile()
{
    unsigned char newcards[2] = {0, 0};
    return createPile(newcards, 0);
}

const Pile *MemoryManager::translatePile(const KCardPile *pile, bool reverse)
{
    unsigned char newcards[MAX_CARDS];
    size_t count = 0;
    size_t len = pile->cards().length();
    for (auto &card : pile->cards()) {
        size_t index = count;
        if (reverse) {
            index = len - count - 1;
        }
        newcards[index] = Card(card).raw_value();
        count++;
    }
    return createPile(newcards, count);
}

}