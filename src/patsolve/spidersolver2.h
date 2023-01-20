/* Copyright (C) 2023 Stephan Kulow <coolo@kde.org>
  SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SPIDERSOLVER2_H
#define SPIDERSOLVER2_H

#include "solverinterface.h"
class Spider;
class Deck;

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
};

#endif
