/*
 * Copyright (C) 1997 Rodolfo Borges <barrett@labma.ufrj.br>
 * Copyright (C) 1998-2009 Stephan Kulow <coolo@kde.org>
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

#include "freecell.h"

#include "deck.h"
#include "speeds.h"
#include "patsolve/freecell.h"

#include <KDebug>
#include <KLocale>

#include <QtCore/QList>
#include <QtCore/QTimer>


const int CHUNKSIZE = 100;

void FreecellPile::moveCards(CardList &c, Pile *to)
{
    if (c.count() == 1) {
        Pile::moveCards(c, to);
        return;
    }
    Freecell *b = dynamic_cast<Freecell*>(dscene());
    if (b) {
        b->moveCards(c, this, to);
    }
}

//-------------------------------------------------------------------------//

Freecell::Freecell()
    : DealerScene()
{
    Deck::createDeck(this, 1);
    Deck::deck()->hide();

    const qreal topRowDist = 1.08;
    const qreal bottomRowDist = 1.13;
    const qreal targetOffsetDist = ( 7 * bottomRowDist + 1 ) - ( 3 * topRowDist + 1 );


    kDebug(11111) << "cards" << Deck::deck()->cards().count();
    for (int i = 0; i < 8; i++)
    {
        FreecellPile *p = new FreecellPile(1 + i, this);
        store[i] = p;
        p->setPilePos( bottomRowDist * i, 1.3 );
        p->setObjectName( QString( "store%1" ).arg( i ) );
        p->setAddFlags(Pile::addSpread | Pile::several);
        p->setRemoveFlags(Pile::several);
        p->setCheckIndex(0);
        p->setReservedSpace( QSizeF( 1.0, 3.5 ) );
    }

    for (int i = 0; i < 4; i++)
    {
        freecell[i] = new Pile (1 + 8 +i, this);
        freecell[i]->setPilePos(topRowDist * i, 0);
        freecell[i]->setObjectName( QString( "freecell%1" ).arg( i ) );
        freecell[i]->setType(Pile::FreeCell);
    }

    for (int i = 0; i < 4; i++)
    {
        target[i] = new Pile(1 + 8 + 4 +i, this);
        target[i]->setObjectName( QString( "target%1" ).arg( i ) );
        target[i]->setPilePos(targetOffsetDist + topRowDist * i, 0);
        target[i]->setType(Pile::KlondikeTarget);
    }

    setActions(DealerScene::Demo | DealerScene::Hint);
    setSolver( new FreecellSolver( this ) );
    setNeededFutureMoves( 4 ); // reserve some
}

Freecell::~Freecell()
{
}
//-------------------------------------------------------------------------//

void Freecell::restart()
{
    Deck::deck()->collectAndShuffle();
    deal();
}

void Freecell::countFreeCells(int &free_cells, int &free_stores) const
{
    free_cells = 0;
    free_stores = 0;

    for (int i = 0; i < 4; i++)
        if (freecell[i]->isEmpty()) free_cells++;
    for (int i = 0; i < 8; i++)
        if (store[i]->isEmpty()) free_stores++;
}

void Freecell::moveCards(CardList &c, FreecellPile *from, Pile *to)
{
    Q_ASSERT(c.count() > 1);
    setWaiting(true);

    from->moveCardsBack(c);
    waitfor = c.first();
    connect(waitfor, SIGNAL(stopped(Card*)), SLOT(waitForMoving(Card*)));

    PileList fcs;

    for (int i = 0; i < 4; i++)
        if (freecell[i]->isEmpty()) fcs.append(freecell[i]);

    PileList fss;

    for (int i = 0; i < 8; i++)
        if (store[i]->isEmpty() && to != store[i]) fss.append(store[i]);

    if (fcs.count() == 0) {
        Q_ASSERT(fss.count());
        fcs.append(fss.last());
        fss.erase(--fss.end());
    }
    while (moves.count()) { delete moves.first(); moves.erase(moves.begin()); }

    sum_moves = 0;
    current_weight = 1000;
    movePileToPile(c, to, fss, fcs, 0, c.count(), 0);

    if (!waitfor->animated())
        QTimer::singleShot(0, this, SLOT(startMoving()));
}

struct MoveAway {
    Pile *firstfree;
    int start;
    int count;
};

void Freecell::movePileToPile(CardList &c, Pile *to, PileList fss, PileList &fcs, int start, int count, int debug_level)
{
    kDebug(11111) << debug_level << "movePileToPile" << c.count() << " " << start  << " " << count;
    int moveaway = 0;
    if (count > fcs.count() + 1) {
        moveaway = (fcs.count() + 1);
        while (moveaway * 2 < count)
            moveaway <<= 1;
    }
    kDebug(11111) << debug_level << "moveaway" << moveaway;

    QList<MoveAway> moves_away;

    if (count - moveaway < (fcs.count() + 1) && (count <= 2 * (fcs.count() + 1))) {
        moveaway = count - (fcs.count() + 1);
    }
    while (count > fcs.count() + 1) {
        Q_ASSERT(fss.count());
        MoveAway ma;
        ma.firstfree = fss[0];
        ma.start = start;
        ma.count = moveaway;
        moves_away.append(ma);
        fss.erase(fss.begin());
        movePileToPile(c, ma.firstfree, fss, fcs, start, moveaway, debug_level + 1);
        start += moveaway;
        count -= moveaway;
        moveaway >>= 1;
        if ((count > (fcs.count() + 1)) && (count <= 2 * (fcs.count() + 1)))
            moveaway = count - (fcs.count() + 1);
    }
    int moving = qMin(count, qMin(c.count() - start, fcs.count() + 1));
    Q_ASSERT(moving);

    for (int i = 0; i < moving - 1; i++) {
        moves.append(new MoveHint(c[c.count() - i - 1 - start], fcs[i], current_weight));
        sum_moves += current_weight;
        current_weight *= 0.9;
    }
    moves.append(new MoveHint(c[c.count() - start - 1 - (moving - 1)], to, current_weight));
    sum_moves += current_weight;
    current_weight *= 0.9;

    for (int i = moving - 2; i >= 0; --i) {
        moves.append(new MoveHint(c[c.count() - i - 1 - start], to, current_weight));
        sum_moves += current_weight;
        current_weight *= 0.9;
    }

    while (moves_away.count())
    {
        MoveAway ma = moves_away.last();
        moves_away.erase(--moves_away.end());
        movePileToPile(c, to, fss, fcs, ma.start, ma.count, debug_level + 1);
        fss.append(ma.firstfree);
    }
}

void Freecell::startMoving()
{
    kDebug(11111) << "startMoving\n";
    if (moves.isEmpty()) {
        takeState();
        return;
    }

    setWaiting(true);
    MoveHint *mh = moves.first();
    moves.erase(moves.begin());
    CardList empty;
    empty.append(mh->card());
    Q_ASSERT(mh->card() == mh->card()->source()->top());
    Q_ASSERT(mh->pile()->legalAdd(empty));
    mh->pile()->add(mh->card());

    int duration = qMax( DURATION_MOVEBACK * mh->priority() / 1000, 1 );
    mh->pile()->moveCardsBack(empty, duration );
    waitfor = mh->card();
    kDebug(11111) << "wait for moving end" << mh->card()->rank() << " " << mh->card()->suit() << mh->priority();
    connect(mh->card(), SIGNAL(stopped(Card*)), SLOT(waitForMoving(Card*)));
    delete mh;
}

void Freecell::newDemoMove(Card *m)
{
    DealerScene::newDemoMove(m);
    if (m != m->source()->top())
        m->disconnect();
}

void Freecell::waitForMoving(Card *c)
{
    if (waitfor != c)
        return;
    c->disconnect(this);
    startMoving();
    setWaiting(false);
}

bool Freecell::cardDblClicked(Card *c)
{
    // target move
    if (DealerScene::cardDblClicked(c))
        return true;

    if (c->animated())
        return false;

    if (c == c->source()->top() && c->realFace())
        for (int i = 0; i < 4; i++)
            if (freecell[i]->isEmpty()) {
                CardList empty;
                empty.append(c);
                c->source()->moveCards(empty, freecell[i]);
                return true;
            }
    return false;
}

bool Freecell::CanPutStore(const Pile *c1, const CardList &c2) const
{
    int fcs, fss;
    countFreeCells(fcs, fss);

    if (c1->isEmpty()) // destination is empty
        fss--;

    if (int(c2.count()) > ((fcs)+1)<<fss)
        return false;

    // ok if the target is empty
    if (c1->isEmpty())
        return true;

    Card *c = c2.first(); // we assume there are only valid sequences

    // ok if in sequence, alternate colors
    return ((c1->top()->rank() == (c->rank()+1))
            && (c1->top()->isRed() != c->isRed()));
}

bool Freecell::checkAdd(int, const Pile *c1, const CardList &c2) const
{
    return CanPutStore(c1, c2);
}

//-------------------------------------------------------------------------//

bool Freecell::checkRemove(int checkIndex, const Pile *p, const Card *c) const
{
    if (checkIndex != 0)
        return false;

    // ok if just one card
    if (c == p->top())
        return true;

    // Now we're trying to move two or more cards.

    // First, let's check if the column is in valid
    // (that is, in sequence, alternated colors).
    int index = p->indexOf(c) + 1;
    const Card *before = c;
    while (true)
    {
        c = p->at(index++);

        if (!((c->rank() == (before->rank()-1))
              && (c->isRed() != before->isRed())))
        {
            return false;
        }
        if (c == p->top())
            return true;
        before = c;
    }

    return true;
}

void Freecell::getHints()
{
    getSolverHints();

    if ( demoActive() )
        return;

    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        Pile *store = *it;
        if (store->isEmpty())
            continue;
//        kDebug(11111) << "trying" << store->top()->name();

        CardList cards = store->cards();
        while (cards.count() && !cards.first()->realFace()) cards.erase(cards.begin());

        CardList::Iterator iti = cards.begin();
        while (iti != cards.end())
        {
            if (store->legalRemove(*iti)) {
//                kDebug(11111) << "could remove" << (*iti)->name();
                for (PileList::Iterator pit = piles.begin(); pit != piles.end(); ++pit)
                {
                    Pile *dest = *pit;
                    if (dest == store)
                        continue;
                    if (store->indexOf(*iti) == 0 && dest->isEmpty() && !dest->target())
                        continue;
                    if (!dest->legalAdd(cards))
                        continue;
                    if ( dest->target() ) // taken care by solver
                        continue;

                    bool old_prefer = checkPrefering( dest->checkIndex(), dest, cards );
                    store->hideCards(cards);
                    // if it could be here as well, then it's no use
                    if ((store->isEmpty() && !dest->isEmpty()) || !store->legalAdd(cards))
                        newHint(new MoveHint(*iti, dest, 0));
                    else {
                        if (old_prefer && !checkPrefering( store->checkIndex(),
                                                           store, cards ))
                        { // if checkPrefers says so, we add it nonetheless
                            newHint(new MoveHint(*iti, dest, 0));
                        }
                    }
                    store->unhideCards(cards);
                }
            }
            cards.erase(iti);
            iti = cards.begin();
        }
    }
}

//-------------------------------------------------------------------------//

void Freecell::deal()
{
    int column = 0;
    while (!Deck::deck()->isEmpty())
    {
        Card *c = Deck::deck()->nextCard();
        store[column]->add (c, false);
        column = (column + 1) % 8;
    }
}

static class LocalDealerInfo3 : public DealerInfo
{
public:
    LocalDealerInfo3() : DealerInfo(I18N_NOOP("Freecell"), 3) {}
    virtual DealerScene *createGame() const { return new Freecell(); }
} ldi8;

//-------------------------------------------------------------------------//

#include "freecell.moc"
