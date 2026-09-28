/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2026 Sam Streeper <ottovox@gmail.com>
 *
 * License of original code:
 * -------------------------------------------------------------------------
 *   Permission to use, copy, modify, and distribute this software and its
 *   documentation for any purpose and without fee is hereby granted,
 *   provided that the above copyright notice appear in all copies and that
 *   both that copyright notice and this permission notice appear in
 *   supporting documentation.
 *
 *   This file is provided AS IS with no warranties of any kind.  The author
 *   shall have no liability with respect to the infringement of copyrights,
 *   trade secrets or any patents by this file or any part thereof.  In no
 *   event will the author be liable for any lost revenue or profits or
 *   other special, indirect and consequential damages.
 * -------------------------------------------------------------------------
 *
 * License of modifications/additions made after 2009-01-01:
 * -------------------------------------------------------------------------
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * -------------------------------------------------------------------------
 */

#ifndef BIZILIZI_H
#define BIZILIZI_H

#define ENABLE_ANALYSIS_WATCHDOG

#include "biziutils.h"
#include "dealer.h"

class BiziPile;
class KSelectAction;

struct CostBenefitChain {
    //  Chain is led by the first _pool_ card sought
    //  So we can seek King first chains for empty piles
    int fpRank; // firstPoolRank (Highest Card in chain)
    int fpColor; // firstPoolColor
    KCard *dc; // deepest card, the goal
    uint16_t cost; // how many non-free cards chain cost
    uint16_t benefit; // how many cards unlocked (+1)
    int priority;
};

struct ChainState {
    std::array<char, 14> blackOdd;
    std::array<char, 14> blackEven;
    void reset();
};

class Bizilizi : public DealerScene
{
    Q_OBJECT

public:
    explicit Bizilizi(const DealerInfo *di);
    void initialize() override;
    void mapOldId(int id) override;
    int oldId() const override;
    QList<QAction *> configActions() const override;
    mutable bool costBenefitHintsPlaced;

protected:
    QString getGameOptions() const override;
    void setGameOptions(const QString &options) override;
    bool checkAdd(const PatPile *pile, const QList<KCard *> &oldCards, const QList<KCard *> &newCards) const override;
    bool checkRemove(const PatPile *pile, const QList<KCard *> &cards) const override;

    void resetRideHintsTimer();
    void resetPoolAnalysisTimer();
    void resetHintTimers();
#ifdef ENABLE_ANALYSIS_WATCHDOG
    void resetWatchdogTimer();
#endif
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void cardsMoved(const QList<KCard *> &cards, KCardPile *oldPile, KCardPile *newPile) override;
    bool pickBuriedSingleton(const KCardPile *pile) const override;
    void restart(const QList<KCard *> &cards) override;
    CardStateArray<uint8_t> upCards;
    int downCardsCount;
    int rootKingsCount;
#ifdef ENABLE_ANALYSIS_WATCHDOG
    bool animationHappened;
#endif

protected Q_SLOTS:
    void animationDone() override;
#ifdef ENABLE_ANALYSIS_WATCHDOG
    void cardStateChanged();
#endif

private Q_SLOTS:
    // menus
    void gameTypeChanged();
    void setRideHints();
    void setPoolAnalysis();
    // timers
    void showDelayedHints();
    void showPoolAnalysis();
#ifdef ENABLE_ANALYSIS_WATCHDOG
    void poolAnalysisWatchdog();
#endif

private:
    void setExtra3(bool extra3);
    bool isBlackOddChain(int rank, int color);
    bool isBlackOddChain(const KCard *c);
    bool isSameChain(const KCard *c1, const KCard *c2);

    QList<const KCard *> getFirstBlackAndRedKing();
    bool findChain(CostBenefitChain &chain, ChainState &cs);
    void findAllChains(QList<CostBenefitChain> &allCBchains);

    // Standard game deals an extra 2 cards compared to Klondike;
    //  23 covered, 22 revealed
    // 'extraThree' deals 3 extra cards down compared to Klondike;
    //  24 covered, 21 revealed
    bool extra3;
    bool easyRules;
    int peekPoolAnalysis;
    int rideHints;

    KSelectAction *options;
    KSelectAction *enablePoolAnalysis;
    KSelectAction *autoHintsUI;

    BiziPile *bsource;
    PatPile *play[7];
    PatPile *target[4];

    QTimer *rideHintTimer;
    QTimer *poolAnalysisTimer;
#ifdef ENABLE_ANALYSIS_WATCHDOG
    QTimer *watchdogTimer;
#endif

    friend class BiziliziSolver;
};

class BiziPile : public PatPile
{
public:
    BiziPile(DealerScene *scene, int index, const QString &objectName = QString());
    QList<QPointF> cardPositions() const override;

    mutable ChainState cs;
};

#endif // BIZILIZI_H
