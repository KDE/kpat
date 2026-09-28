/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2010 Parker Coates <coates@kde.org>
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

/*
 * Originally derived from Klondike by the original authors.
 * Thank you!
 * Recoded as Bizilizi by Streeper, bugs if any are mine
 */

#include "bizilizi.h"

// own
#include "dealerinfo.h"
#include "patsolve/bizilizisolver.h"
#include "pileutils.h"
#include "settings.h"
#include "speeds.h"
// KF
#include <KLazyLocalizedString>
#include <KLocalizedString>
#include <KSelectAction>

const qreal bizipileYsame = 0.28;
const qreal bizipileYdiff = 0.5;

// memo: call it pool, source?
BiziPile::BiziPile(DealerScene *scene, int index, const QString &objectName)
    : PatPile(scene, index, objectName)
{
}

namespace
{
inline int alternateColor(int color)
{
    return color == KCardDeck::Red ? KCardDeck::Black : KCardDeck::Red;
}
} // end namespace

QList<QPointF> BiziPile::cardPositions() const
{
    // width of play tableau, bizipile width may match this
    // this is really 7 cards + 6*hspacing, how to make clear in all code?
    qreal playWidth = 8.1; // was 7.5
    qreal bwidth = playWidth;
    // 12 cards that should overlap in a chain, but not overlap in breaks
    qreal bspread = (bwidth - 1.0) / 10;
    int curRank = 0; // no rank yet

    QList<QPointF> positions;
    QPointF currentPos(0, 0);
    int prevColor = -1;
    bool blackFirst = true;

    cs.reset();

    const auto cards = this->cards();
    for (KCard *c : cards) {
        int rank = c->rank();
        if (curRank != rank) {
            curRank = rank;
            currentPos = QPointF(bspread * (KCardDeck::King - rank), 0);
            prevColor = -1;
            blackFirst = rank % 2; // odd, club/spade first for King(13)
        }

        if (blackFirst) {
            // odd, black first
            if (c->color() == KCardDeck::Black) {
                currentPos.setY((prevColor != KCardDeck::Black) ? 0 : bizipileYsame);
                cs.blackOdd[rank] = 1;
            } else {
                // red card
                currentPos.setY((prevColor != KCardDeck::Red) ? bizipileYdiff : bizipileYdiff + bizipileYsame);
                cs.blackEven[rank] = 1;
            }
        } else {
            // even, red first
            if (c->color() == KCardDeck::Red) {
                currentPos.setY((prevColor != KCardDeck::Red) ? 0 : bizipileYsame);
                cs.blackOdd[rank] = 1;
            } else {
                // black card
                currentPos.setY((prevColor != KCardDeck::Black) ? bizipileYdiff : bizipileYdiff + bizipileYsame);
                cs.blackEven[rank] = 1;
            }
        }
        positions << currentPos;
        prevColor = c->color();
    }

    //  we are here because sendCardsToPile has asked for this pile's positions.
    //  sendCardsToPile will animate the cards, and when finished all card rotations will be 0
    //  Pool analysis peeking will rotate pool chain leaders after card pile spacing animation has finished.
    dynamic_cast<Bizilizi *>(scene())->costBenefitHintsPlaced = false;

    return positions;
}

Bizilizi::Bizilizi(const DealerInfo *di)
    : DealerScene(di)
{
}

/* patpile   index : name : purpose
 1-4 : target0-target3 : ace buildup piles "foundation"
 5-11: play0 - play6 : 7 playing builddown piles "tableau"
 13  : bsource  : free play cards, ranked piles K-2 "waste"
*/

void Bizilizi::initialize()
{
    // The units of the following constants are card dimension units
    const qreal hspacing = 0.185; // horizontal spacing between card piles
    const qreal vspacing = 0.085; // vertical spacing between card piles

    setDeckContents();

    // only affects target pullback at this time, and not game-settable -s
    easyRules = false;
    extra3 = Settings::biziliziIsExtra3();
    peekPoolAnalysis = Settings::poolAnalysisEnabled();
    rideHints = Settings::rideHints();

    qreal xOffset = 0;
    qreal yOffset = 3.4;
    bsource = new BiziPile(this, 13, QStringLiteral("bsource"));
    bsource->setPileRole(PatPile::Waste);
    bsource->setBottomPadding(0.68);
    bsource->setRightPadding(7.6);
    bsource->setHeightPolicy(KCardPile::GrowDown);
    bsource->setLayoutPos(xOffset, yOffset);
    // bsource->setSpread(0.55, 0);  // not currently used
    bsource->setKeyboardSelectHint(KCardPile::AutoFocusDeepestFaceUp);
    bsource->setKeyboardDropHint(KCardPile::NeverFocus);

    xOffset = 0.1;
    yOffset = 0;
    for (int i = 0; i < 7; ++i) {
        // index 5-11, "play0"-"play6"
        play[i] = new PatPile(this, i + 5, QStringLiteral("play%1").arg(i));
        play[i]->setPileRole(PatPile::Tableau);
        play[i]->setLayoutPos(xOffset + ((1.0 + hspacing) * i), yOffset);
        play[i]->setAutoTurnTop(true);
        play[i]->setBottomPadding(play[i]->spread().y() * 4.4);
        play[i]->setHeightPolicy(KCardPile::GrowDown);
        play[i]->setKeyboardSelectHint(KCardPile::AutoFocusDeepestFaceUp);
        play[i]->setKeyboardDropHint(KCardPile::AutoFocusTop);
    }

    xOffset = 8.7;
    yOffset = 0.066;
    int x = 0, i = 0;
    for (int y = 0; y < 4; y++) {
        // index 1-4, "target0"-"target3"
        target[i] = new PatPile(this, i + 1, QStringLiteral("target%1").arg(i));
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setLayoutPos(xOffset + (1.0 + hspacing) * x, yOffset + (1.0 + vspacing) * y);
        target[i]->setKeyboardSelectHint(KCardPile::ForceFocusTop);
        target[i]->setKeyboardDropHint(KCardPile::ForceFocusTop);
        i++;
    }

    setActions(DealerScene::Hint | DealerScene::Demo);

    setSolver(new BiziliziSolver(this));
    setNeededFutureMoves(21);

    options = new KSelectAction(i18nc("@title:menu", "Bizilizi &Options"), this);
    options->addAction(i18nc("@item:inmenu bizilizi option", "23 Covered Cards"));
    options->addAction(i18nc("@item:inmenu bizilizi option", "24 Covered Cards"));
    options->setCurrentItem(extra3 ? 1 : 0);
    connect(options, &KSelectAction::indexTriggered, this, &Bizilizi::gameTypeChanged);

    enablePoolAnalysis = new KSelectAction(i18nc("@title:menu", "Pool Analysis"), this);
    enablePoolAnalysis->addAction(i18nc("@item:inmenu analysis option", "Off"));
    enablePoolAnalysis->addAction(i18nc("@item:inmenu analysis option", "Instant"));
    enablePoolAnalysis->addAction(i18nc("@item:inmenu analysis option", "Wait 4"));
    enablePoolAnalysis->addAction(i18nc("@item:inmenu analysis option", "Wait 10"));
    enablePoolAnalysis->addAction(i18nc("@item:inmenu analysis option", "Wait 18"));
    enablePoolAnalysis->setCurrentItem(peekPoolAnalysis);
    connect(enablePoolAnalysis, &KSelectAction::indexTriggered, this, &Bizilizi::setPoolAnalysis);

    autoHintsUI = new KSelectAction(i18nc("@title:menu", "Delayed Hints"), this);
    autoHintsUI->addAction(i18nc("@item:inmenu ridehints option", "Off"));
    autoHintsUI->addAction(i18nc("@item:inmenu ridehints option", "6 Seconds"));
    autoHintsUI->addAction(i18nc("@item:inmenu ridehints option", "14 Seconds"));
    autoHintsUI->addAction(i18nc("@item:inmenu ridehints option", "22 Seconds"));
    autoHintsUI->setCurrentItem(rideHints);
    connect(autoHintsUI, &KSelectAction::indexTriggered, this, &Bizilizi::setRideHints);

    rideHintTimer = new QTimer(this);
    rideHintTimer->setSingleShot(true);
    connect(rideHintTimer, &QTimer::timeout, this, &Bizilizi::showDelayedHints);

    poolAnalysisTimer = new QTimer(this);
    poolAnalysisTimer->setSingleShot(true);
    connect(poolAnalysisTimer, &QTimer::timeout, this, &Bizilizi::showPoolAnalysis);

#ifdef ENABLE_ANALYSIS_WATCHDOG
    watchdogTimer = new QTimer(this);
    watchdogTimer->setSingleShot(true);
    connect(watchdogTimer, &QTimer::timeout, this, &Bizilizi::poolAnalysisWatchdog);

    connect(this, &DealerScene::updateMoves, this, &Bizilizi::cardStateChanged);
#endif
}

bool Bizilizi::checkAdd(const PatPile *pile, const QList<KCard *> &oldCards, const QList<KCard *> &newCards) const
{
    switch (pile->pileRole()) {
    case PatPile::Tableau:
        return checkAddAlternateColorDescendingFromKing(oldCards, newCards);
    case PatPile::Foundation:
        return checkAddSameSuitAscendingFromAce(oldCards, newCards);
    case PatPile::Waste:
    default:
        return false;
    }
}

bool Bizilizi::checkRemove(const PatPile *pile, const QList<KCard *> &cards) const
{
    switch (pile->pileRole()) {
    case PatPile::Tableau:
        return isAlternateColorDescending(cards);
    case PatPile::Foundation:
        // no pullback from foundation at this time...
        return easyRules && cards.first() == pile->topCard();
    case PatPile::Waste:
        return true;
    default:
        return false;
    }
}

void Bizilizi::resetRideHintsTimer()
{
    if (rideHints) {
        int delay;
        switch (rideHints) {
        case 1:
            delay = 6023;
            break;
        case 2:
            delay = 14012;
            break;
        case 3:
            delay = 22012;
            break;
        default: // unused
            delay = 1;
        }
        rideHintTimer->start(delay);
    } else
        rideHintTimer->stop();
}

void Bizilizi::resetPoolAnalysisTimer()
{
    int delay = 1;
    if (peekPoolAnalysis) {
        switch (peekPoolAnalysis) {
        case 2:
            delay = 4000;
            break;
        case 3:
            delay = 10123;
            break;
        case 4:
            delay = 17600;
            break;
        }
    }

    if (isDemoActive() || isDropActive()) {
        // suppress analysis until after drop finishes AND times out
        // TIME_BETWEEN_MOVES = 250ms, cardMoveDuration = 230 (ish)
        // 350 not quite enough
        poolAnalysisTimer->start(delay + TIME_BETWEEN_MOVES + 365);
#ifdef ENABLE_ANALYSIS_WATCHDOG
        watchdogTimer->stop();
#endif
        return;
    }

    if (peekPoolAnalysis) {
        poolAnalysisTimer->start(delay);
    } else
        poolAnalysisTimer->stop();
}

void Bizilizi::resetHintTimers()
{
    resetPoolAnalysisTimer();
    resetRideHintsTimer();
}

#ifdef ENABLE_ANALYSIS_WATCHDOG
void Bizilizi::resetWatchdogTimer()
{
    // long enough that there's a problem if animationHappened didn't happen
    watchdogTimer->start(640);
}
#endif

void Bizilizi::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
    DealerScene::mousePressEvent(e);
    // delay hinting on a mouse event, but don't redo quick pool analysis peeking
    if (peekPoolAnalysis > 1)
        resetHintTimers();
    else
        resetRideHintsTimer();
}
void Bizilizi::cardsMoved(const QList<KCard *> &cards, KCardPile *oldPile, KCardPile *newPile)
{
    DealerScene::cardsMoved(cards, oldPile, newPile);

    if (oldPile == bsource)
        setNeededFutureMoves(bsource->count());
    // poolAnalysisTimer will be reset momentarily if peekPoolAnalysis
    resetRideHintsTimer();

    // changes to the tableau force rebuild of pool hints
    if (peekPoolAnalysis)
        updatePileLayout(bsource, 0);
}

// slot connected to rideHintTimer
void Bizilizi::showDelayedHints()
{
    startHint();
}

// slot connected to poolAnalysisTimer
void Bizilizi::showPoolAnalysis()
{
    if (peekPoolAnalysis && !costBenefitHintsPlaced) {
        QList<CostBenefitChain> allCBchains;
        findAllChains(allCBchains);

        if (downCardsCount == 0) {
            // no covered cards, no reason to continue pool analysis
            return;
        }

        // make a map of all pool cards that match cb chain leaders.
        CardStateArray<KCard *> pool;
        for (KCard *c : bsource->cards()) {
            pool.set(c, c); // quick map so I can look up pool cards by (rank,suit)
        }

        // Iterate chains, Make map of first (highest) benefit to chain leaders,
        //   count distinct applied cbchains as divisor for priority peek effect
        CardStateMap<CostBenefitChain *> benefit;
        int benefitCount = 0;
        uint16_t oldCost{0xFFFF}, oldBen{0xFFFF};
        for (auto &cb : allCBchains) {
            // color is a synonym for Clubs | Diamonds here, just one card mapped
            // since benefits are sorted, only the first (highest priority) benefit gets applied to a link
            if (!benefit.get(cb.fpRank, cb.fpColor)) {
                if (cb.cost != oldCost || cb.benefit != oldBen) {
                    benefitCount++;
                    oldCost = cb.cost;
                    oldBen = cb.benefit;
                }
                cb.priority = benefitCount;
                benefit.set(cb.fpRank, cb.fpColor, &cb);
            }
        }

        // Iterate pool, queue animation with relevant priority to (both) link cards
        for (KCard *c : bsource->cards()) {
            auto fx = 1.0; // scale the peek effect to the costBenefit
            // Since the cost benefit chain is linked to color, it could have been mapped to either
            // this card or its color mate, but both get the peekHint if present.
            // But we still need to still need to fetch the CostBenefitChain to get its
            // priority and set the peekHint scaling.
            auto ben1 = benefit.get(c->rank(), c->suit());
            // tricky: XOR with 3 flips suit to its colormate
            auto ben2 = benefit.get(c->rank(), c->suit() ^ 3);
            CostBenefitChain *actualBenefit = nullptr;

            if (ben1.has_value())
                actualBenefit = *ben1;
            else if (ben2.has_value())
                actualBenefit = *ben2;

            if (actualBenefit) {
                // highest benefit priority is 1, then 2, 3... so with 3 distinct benefit values,
                // benefits will be 3/3, 2/3, 1/3 and scale the peek effect for pool analysis
                fx = (benefitCount + 1.0 - actualBenefit->priority) / benefitCount;

                static const auto WDIV{4.7};
                static const auto HDIV{6.5};
                QPointF pos2(c->x() + (fx * deck()->cardWidth() / WDIV), c->y() - (fx * deck()->cardHeight() / HDIV));

                static const auto ROT_DEG{5.2};
                static const auto DURATION_MS{60};
                c->animate(pos2, c->zValue(), fx * ROT_DEG, true, false, DURATION_MS);
            }
        }
        costBenefitHintsPlaced = true;
    }
}

bool Bizilizi::pickBuriedSingleton(const KCardPile *pile) const
{
    const PatPile *p = dynamic_cast<const PatPile *>(pile);

    switch (p->pileRole()) {
    case PatPile::Waste:
        return true;
    default:
        return false;
    }
}

QList<QAction *> Bizilizi::configActions() const
{
    return QList<QAction *>() << options << enablePoolAnalysis << autoHintsUI;
}

// Sorted position varies based on whether rank is even/odd
// so that colors alternate to display potential chains in
// the bsource pile
//...............eC  D  H  S oC  D  H  S
QList<int> altPos{1, 3, 2, 0, 3, 1, 0, 2};
//...cards hash out D>H>C>S     C>S>D>H

static inline int alernatingCardHash(const KCard *c)
{
    auto rank = c->rank();
    auto odd = rank % 2;
    auto suit = c->suit();

    return (rank * 4) + altPos[(odd * 4) + suit];
}

static bool alternatingCardGreaterThan(const KCard *a, const KCard *b)
{
    return alernatingCardHash(a) > alernatingCardHash(b);
}

void Bizilizi::restart(const QList<KCard *> &cards)
{
    QList<KCard *> cardList = cards;

    for (int i = 0; i < (extra3 == 0 ? 2 : 3); ++i)
        addCardForDeal(play[i], cardList.takeLast(), 0, play[0]->pos());

    for (int round = 0; round < 7; ++round)
        for (int i = round; i < 7; ++i)
            addCardForDeal(play[i], cardList.takeLast(), (i == round), play[0]->pos());

    std::sort(cardList.begin(), cardList.end(), alternatingCardGreaterThan);

    while (!cardList.isEmpty()) {
        // takeFirst to preserve suit order; this is a dump, not a deal
        KCard *c = cardList.takeFirst();
        c->setPos(target[0]->pos());
        c->setFaceUp(true);

        int rank = c->rank();
        if (rank == KCardDeck::Ace) {
            //  fixme: remap suit to pile location?
            addCardForDeal(target[c->suit()], c, 1, play[0]->pos());
        } else {
            addCardForDeal(bsource, c, 1, play[0]->pos());
        }
    }
    startDealAnimation();
    resetRideHintsTimer();
}

// slot for menu signal
void Bizilizi::gameTypeChanged()
{
    stopDemo();

    if (allowedToStartNewGame()) {
        setExtra3(options->currentItem() == 1);
        startNew(gameNumber());
    } else {
        // If we're not allowed, reset the option to the
        // current number of extra cards down compared to klondike.
        options->setCurrentItem(extra3 ? 1 : 0);
    }
}

// slot for menu signal
void Bizilizi::setRideHints()
{
    rideHints = autoHintsUI->currentItem();
    if (rideHints)
        startHint();
    else {
        rideHintTimer->stop();
        stopHint();
    }
    resetRideHintsTimer();
    Settings::setRideHints(rideHints);
}

// slot for menu signal
void Bizilizi::setPoolAnalysis()
{
    peekPoolAnalysis = enablePoolAnalysis->currentItem();
    resetPoolAnalysisTimer();
    updatePileLayout(bsource, 0);
    Settings::setPoolAnalysisEnabled(peekPoolAnalysis);
}

// options for a save game file
QString Bizilizi::getGameOptions() const
{
    return QString::number(extra3 ? 1 : 0);
}

// take options from a saved game
void Bizilizi::setGameOptions(const QString &options)
{
    setExtra3(options.toInt() == 1);
}

void Bizilizi::setExtra3(bool _Extra3)
{
    if (_Extra3 != extra3) {
        extra3 = _Extra3;
        options->setCurrentItem(extra3 ? 1 : 0);
        Settings::setBiziliziIsExtra3(extra3);
    }
}

bool Bizilizi::isBlackOddChain(int rank, int color)
{
    if (color == KCardDeck::Black) {
        if (rank % 2)
            return true;
        return false;
    }
    // card is Red
    if (rank % 2)
        return false;
    return true; // aka red_Even
}

bool Bizilizi::isBlackOddChain(const KCard *c)
{
    return (isBlackOddChain(c->rank(), c->color()));
}

bool Bizilizi::isSameChain(const KCard *c1, const KCard *c2)
{
    bool b1 = isBlackOddChain(c1);
    bool b2 = isBlackOddChain(c2);
    return (b1 == b2);
}

QList<const KCard *> Bizilizi::getFirstBlackAndRedKing()
{
    QList<const KCard *> result;
    if (bsource->isEmpty()) {
        return result;
    }
    bool blackKingFound = false;
    for (const auto c : bsource->cards()) {
        if (c->rank() == KCardDeck::King) {
            if (c->color() == KCardDeck::Black) {
                if (blackKingFound)
                    continue;
                blackKingFound = true;
                result.append(c);
                continue;
            }
            // append red king
            result.append(c);
            break;
        }
        if (c->rank() != KCardDeck::King)
            break;
    }
    return result;
}

bool Bizilizi::findChain(CostBenefitChain &chain, ChainState &cs)
{
    KCard *deepc = chain.dc;
    // KCard *firstc = chain.fc;

    if (deepc->rank() == 1)
        return false;
    if (chain.fpRank <= deepc->rank())
        return false;

    std::array<char, 14> *chainMap = &cs.blackOdd;
    bool blackOdd = true;
    if (!isBlackOddChain(deepc)) {
        chainMap = &cs.blackEven;
        blackOdd = false;
    }

    auto cost = 0;
    for (auto i = deepc->rank() + 1; i <= chain.fpRank; i++) {
        if ((*chainMap)[i] != 1)
            return false;
        // is this connector a freebie?
        int suit;
        if (blackOdd) {
            if (i % 2)
                suit = KCardDeck::Spades; // or clubs, I'll check both
            else
                suit = KCardDeck::Hearts; // or diamonds
        } else { // blackEven chain
            if (i % 2)
                suit = KCardDeck::Hearts; // or diamonds
            else
                suit = KCardDeck::Spades; // or clubs...
        }
        // tricky: use xor for other suit of same color
        if (upCards.get(i, suit) && upCards.get(i, suit ^ 3)) {
            continue; // both chain cards up, its free
        }
        // not risk free card
        cost++;
    }
    // woo hoo, we have a connecting chain!
    chain.cost = cost;
    return true;
}

static bool costBenefitGT(const CostBenefitChain &a, const CostBenefitChain &b)
{
    // sort by minimum cost, then max benefit
    if (a.cost == b.cost || a.benefit == 0 || b.benefit == 0) {
        return a.benefit > b.benefit;
    }
    return a.cost < b.cost;
}

void Bizilizi::findAllChains(QList<CostBenefitChain> &allCBchains)
{
    upCards.reset();

    const auto patPiles = this->patPiles();

    downCardsCount = 0;
    rootKingsCount = 0;
    // record every exposed card
    for (const PatPile *p : patPiles) {
        QList<KCard *> cards = p->cards();
        for (const auto c : cards) {
            if (c->isFaceUp()) {
                upCards.set(c, 1);
            } else
                downCardsCount++;
        }
    }

    // note every deepest face up card and the count below it
    // so we don't have to repeat these queries in a loop
    std::array<KCard *, 7> deepestUp{};
    std::array<int, 7> benefits{};
    for (auto i = 0; i < 7; i++) {
        auto pile = play[i];
        if (!pile->isEmpty()) {
            auto index = pile->count() - 1;
            while (index > 0 && pile->at(index - 1)->isFaceUp())
                index--;
            KCard *c = pile->at(index);
            deepestUp[i] = c;
            // benefit is how many cards will be revealed below
            benefits[i] = index;
            if (c->rank() == KCardDeck::King && index == 0)
                rootKingsCount++;
        }
    }

    // punch all top card analog singletons out of (temp copy) chain connection maps
    // these will never be free cards! because you use the pile card for chain intead of pool card
    // t_cs is temp chainState, modifiable working copy of the chain maps
    auto t_cs = bsource->cs;

    for (auto fc = 0; fc < 7; fc++) {
        auto pile = play[fc];
        if (pile->isEmpty())
            continue;
        KCard *topc = pile->topCard();
        auto ndx = pile->indexOf(topc);
        if ((ndx > 0) && (pile->at(ndx - 1)->isFaceUp()))
            continue;
        // singleton on top, remove it from chain map
        int rank = topc->rank();
        bool blackFirst = rank % 2;

        if (blackFirst) { // card odd
            if (topc->color() == KCardDeck::Black) {
                t_cs.blackOdd[rank] = 0;
            } else {
                // red card
                t_cs.blackEven[rank] = 0;
            }
        } else {
            // card even, red first
            if (topc->color() == KCardDeck::Red) {
                t_cs.blackOdd[rank] = 0; // redEven
            } else {
                // black card
                t_cs.blackEven[rank] = 0; // redOdd
            }
        }
    }

    auto emptyPiles = false;
    for (auto du = 0; du < 7; du++) {
        // all deepestUp cards
        for (auto topc = 0; topc < 7; topc++) {
            // look for chain from all play pile first cards
            if (du == topc)
                continue;
            KCard *deepc = deepestUp[du];
            if (deepc == nullptr)
                continue;
            auto pile = play[topc];
            if (pile->isEmpty()) {
                emptyPiles = true;
                continue;
            }
            KCard *topCard = pile->topCard();
            if (topCard->rank() <= deepc->rank() + 1)
                // dont need chain connecter
                continue;
            if (!isSameChain(deepc, topCard))
                continue;

            CostBenefitChain cbChain{topCard->rank() - 1, alternateColor(topCard->color()), deepc, 0, 0, 0};
            if (findChain(cbChain, t_cs)) {
                if (!(benefits[du] == 0 && rootKingsCount == 4)) {
                    // no benefit to moving rooted card with 4 root kings, otherwise
                    // benefit is one more than cards revealed bc opening a pile has benefit
                    cbChain.benefit = benefits[du] + 1;
                    allCBchains.push_back(cbChain);
                }
            }
        }
    }

    // If empty piles, look for any chains from deepestUp to Black and Red kings in pool
    if (emptyPiles) {
        auto poolKings = getFirstBlackAndRedKing();
        for (auto du = 0; du < 7; du++) {
            // all deepestUp cards
            for (auto topCard : poolKings) { // topc was int 1-7
                // look for chain from all play pile first cards
                KCard *deepc = deepestUp[du];
                if (deepc == nullptr)
                    continue;
                if (deepc->rank() == KCardDeck::King)
                    // dont need chain connecter
                    continue;
                if (!isSameChain(deepc, topCard))
                    continue;

                CostBenefitChain cbChain{topCard->rank(), topCard->color(), deepc, 0, 0, 0};
                if (findChain(cbChain, t_cs)) {
                    // benefit is one more than cards revealed bc opening a pile has benefit
                    cbChain.benefit = benefits[du] + 1;
                    allCBchains.push_back(cbChain);
                }
            }
        }
    }

    // sort by minimum cost, then max benefit
    std::sort(allCBchains.begin(), allCBchains.end(), costBenefitGT);

#if 0
    // eliminate lower priority (ie later after sort) chains that reuse destination
    for (auto i = 0; i < allCBchains.length() - 1; i++) {
        for (auto j = i + 1; j < allCBchains.length(); j++) {
            auto &c1 = allCBchains[i];
            auto &c2 = allCBchains[j];
            // example: 7S & 7C find pool link thru 6D, 2 cb entries with no difference
            if ((c1.fpRank == c2.fpRank) && (c1.fpColor == c2.fpColor)) {
                //  not really necessary, only first link will make a benefit later
                c2.benefit = 0;
            }
        }
    }
#endif
}

void Bizilizi::animationDone()
{
    DealerScene::animationDone();

    resetPoolAnalysisTimer();
#ifdef ENABLE_ANALYSIS_WATCHDOG
    animationHappened = true;
#endif
}

#ifdef ENABLE_ANALYSIS_WATCHDOG
// slot from updateMoves, since undo/redo change board state without triggering animation
// watchdog will trigger pool layout/animation to update poolAnalysis in this case
void Bizilizi::cardStateChanged()
{
    animationHappened = false;
    resetWatchdogTimer();
    resetRideHintsTimer(); // delay hints so they don't fire amid multiple undos
    // redelay pool analysis post-deal if delay set
    if (peekPoolAnalysis > 1 && moveCount() > 0)
        resetPoolAnalysisTimer();
}

// slot connected to watchdogTimer
void Bizilizi::poolAnalysisWatchdog()
{
    if (!isCardAnimationRunning() && !animationHappened) {
        // undo/redo changed board but didn't trigger animation / poolAnalysis
        updatePileLayout(bsource, 0); // resets old hints
        resetHintTimers(); // will animate current hints
    }
}
#endif

void Bizilizi::mapOldId(int id)
{
    switch (id) {
    case DealerInfo::BiziliziExtra2Id:
        setExtra3(false);
        break;
    case DealerInfo::BiziliziExtra3Id:
        setExtra3(true);
        break;
    case DealerInfo::BiziliziId:
    default:
        // Do nothing.
        break;
    }
}

int Bizilizi::oldId() const
{
    if (extra3)
        return DealerInfo::BiziliziExtra3Id;
    else
        return DealerInfo::BiziliziExtra2Id;
}

static class BiziliziDealerInfo : public DealerInfo
{
public:
    BiziliziDealerInfo()
        : DealerInfo(kli18n("Bizilizi"),
                     BiziliziId,
                     {
                         {BiziliziExtra2Id, kli18n("Bizilizi (23 Covered)")},
                         {BiziliziExtra3Id, kli18n("Bizilizi (24 Covered)")},
                     })
    {
    }

    DealerScene *createGame() const override
    {
        return new Bizilizi(this);
    }
} biziliziDealerInfo;

void ChainState::reset()
{
    blackOdd.fill(0);
    blackEven.fill(0);
}

#include "moc_bizilizi.cpp"
