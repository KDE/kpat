/*
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2010 Parker Coates <coates@kde.org>
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

#include "simon.h"

// own
#include "dealerinfo.h"
#include "patsolve/simonsolver.h"
#include "pileutils.h"
#include "settings.h"
// KF
#include <KLazyLocalizedString>

Simon::Simon(const DealerInfo *di)
    : DealerScene(di)
{
}

void Simon::initialize()
{
    setDeckContents();

    const qreal dist_x = 1.11;

    for (int i = 0; i < 4; ++i) {
        target[i] = new PatPile(this, i + 1, QStringLiteral("target%1").arg(i));
        target[i]->setPileRole(PatPile::Foundation);
        target[i]->setLayoutPos((i + 3) * dist_x, 0);
        target[i]->setSpread(0, 0);
        target[i]->setKeyboardSelectHint(KCardPile::NeverFocus);
        target[i]->setKeyboardDropHint(KCardPile::AutoFocusTop);
    }

    for (int i = 0; i < 10; ++i) {
        store[i] = new PatPile(this, 5 + i, QStringLiteral("store%1").arg(i));
        store[i]->setPileRole(PatPile::Tableau);
        store[i]->setLayoutPos(dist_x * i, 1.2);
        store[i]->setBottomPadding(2.5);
        store[i]->setHeightPolicy(KCardPile::GrowDown);
        store[i]->setZValue(0.01 * i);
        store[i]->setKeyboardSelectHint(KCardPile::AutoFocusDeepestRemovable);
        store[i]->setKeyboardDropHint(KCardPile::AutoFocusTop);
    }

    setActions(DealerScene::Hint | DealerScene::Demo);
    auto solver = new SimonSolver(this);
    solver->default_max_positions = Settings::simpleSimonSolverIterationsLimit();
    setSolver(solver);
    // setNeededFutureMoves( 1 ); // could be some nonsense moves
}

void Simon::restart(const QList<KCard *> &cards)
{
    QList<KCard *> cardList = cards;

    QPointF initPos(0, -deck()->cardHeight());

    for (int piles = 9; piles >= 3; --piles)
        for (int j = 0; j < piles; ++j)
            addCardForDeal(store[j], cardList.takeLast(), true, initPos);

    for (int j = 0; j < 10; ++j)
        addCardForDeal(store[j], cardList.takeLast(), true, initPos);

    Q_ASSERT(cardList.isEmpty());

    startDealAnimation();
}

bool Simon::checkPrefering(const PatPile *pile, const QList<KCard *> &oldCards, const QList<KCard *> &newCards) const
{
    return pile->pileRole() == PatPile::Tableau && !oldCards.isEmpty() && oldCards.last()->suit() == newCards.first()->suit();
}

bool Simon::checkAdd(const PatPile *pile, const QList<KCard *> &oldCards, const QList<KCard *> &newCards) const
{
    if (pile->pileRole() == PatPile::Tableau) {
        if (!(oldCards.isEmpty() || oldCards.last()->rank() == newCards.first()->rank() + 1)) {
            return false;
        }

        int seqs_count = countSameSuitDescendingSequences(newCards);

        if (seqs_count < 0)
            return false;

        // This is similar to the supermoves of Freecell - we can use empty
        // columns to temporarily hold intermediate sub-sequences which are
        // not the same suit - only a "false" parent.
        //      Shlomi Fish

        int empty_piles_count = 0;

        for (int i = 0; i < 10; ++i)
            if (store[i]->isEmpty() && (store[i]->index() != pile->index()))
                empty_piles_count++;

        return (seqs_count <= (1 << empty_piles_count));
    } else {
        return oldCards.isEmpty() && newCards.first()->rank() == KCardDeck::King && newCards.last()->rank() == KCardDeck::Ace && isSameSuitDescending(newCards);
    }
}

bool Simon::checkRemove(const PatPile *pile, const QList<KCard *> &cards) const
{
    if (pile->pileRole() != PatPile::Tableau)
        return false;

    int seqs_count = countSameSuitDescendingSequences(cards);
    return (seqs_count >= 0);
}

QString Simon::solverFormat() const
{
    QString output;
    QString tmp;
    for (int i = 0; i < 4; i++) {
        if (target[i]->isEmpty())
            continue;
        tmp += suitToString(target[i]->topCard()->suit()) + QLatin1String("-K ");
    }
    if (!tmp.isEmpty())
        output += QStringLiteral("Foundations: %1\n").arg(tmp);

    for (int i = 0; i < 10; i++) {
        cardsListToLine(output, store[i]->cards());
    }
    return output;
}

static class SimonDealerInfo : public DealerInfo
{
public:
    SimonDealerInfo()
        : DealerInfo(kli18n("Simple Simon"), SimpleSimonId)
    {
    }

    DealerScene *createGame() const override
    {
        return new Simon(this);
    }
} simonDealerInfo;

#include "moc_simon.cpp"
