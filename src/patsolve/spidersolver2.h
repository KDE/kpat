/* Copyright (C) 2023 Stephan Kulow <coolo@kde.org>
  SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SPIDERSOLVER2_H
#define SPIDERSOLVER2_H

#include "solverinterface.h"
#include <unordered_map>
class Spider;
class KCardPile;

namespace spidersolver2
{
class Deck;
class MemoryManager;

// how many siblings do we expect for a set of decks (not counting dups)
const int AVG_OPTIONS = 50;

// limit the number of paths to visit in each level
// higher cap mostly finds better solutions (or sometimes some at all), but also takes
// way more time
const int LEVEL_CAP = 150;

class SpiderSolver2 : public SolverInterface
{
public:
    SpiderSolver2(Spider *dealer);
    ~SpiderSolver2();
    virtual ExitStatus patsolve(int max_positions = -1) override;
    virtual void translate_layout() override;
    virtual MoveHint translateMove(const MOVE &m) override;

    virtual void stopExecution() override;
    virtual QList<MOVE> firstMoves() const override
    {
        return m_firstMoves;
    }
    virtual QList<MOVE> winMoves() const override
    {
        return m_winMoves;
    }

private:
    int solve();
    Spider *m_dealer;
    Deck *orig;
    QList<MOVE> m_firstMoves;
    QList<MOVE> m_winMoves;
    MemoryManager *memManager;
};
}

#endif
