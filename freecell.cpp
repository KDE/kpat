/*---------------------------------------------------------------------------

  freecell.cpp  implements a patience card game

     Copyright 1997 Rodolfo Borges <barrett@labma.ufrj.br>
     Copyright 2000 Stephan Kulow <coolo@kde.org>

 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.

---------------------------------------------------------------------------*/

#include "freecell.h"
#include <klocale.h>
#include "deck.h"
#include <assert.h>
#include <kdebug.h>
#include <stdlib.h>
#include <QTimer>
#include <QList>
#include <QDomDocument>
#include <QTextStream>
#include <QFile>
#include "patsolve/freecell.h"

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
    Deck::create_deck(this, 1);
    Deck::deck()->hide();

    kDebug(11111) << "cards " << Deck::deck()->cards().count() << endl;
    for (int i = 0; i < 8; i++)
    {
        FreecellPile *p = new FreecellPile(1 + i, this);
        store[i] = p;
        store[i]->setPilePos(1 + 11.3 * i, 14 );
        store[i]->setObjectName( QString( "store%1" ).arg( i ) );
        p->setAddFlags(Pile::addSpread | Pile::several);
        p->setRemoveFlags(Pile::several);
        p->setCheckIndex(0);
        p->setReservedSpace( QSizeF( 10, 28 ) );
    }

    for (int i = 0; i < 4; i++)
    {
        freecell[i] = new Pile (1 + 8 +i, this);
        freecell[i]->setPilePos(1 + 10.8 * i, 1);
        freecell[i]->setObjectName( QString( "freecell%1" ).arg( i ) );
        freecell[i]->setType(Pile::FreeCell);
    }

    for (int i = 0; i < 4; i++)
    {
        target[i] = new Pile(1 + 8 + 4 +i, this);
        target[i]->setObjectName( QString( "target%1" ).arg( i ) );
        target[i]->setPilePos(4 + 10.8 * ( 4 + i ), 1);
        target[i]->setType(Pile::KlondikeTarget);
        // COOLO: I'm still not too sure about that t->setRemoveFlags(Pile::Default);
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
    assert(c.count() > 1);
    setWaiting(true);

    from->moveCardsBack(c);
    waitfor = c.first();
    connect(waitfor, SIGNAL(stoped(Card*)), SLOT(waitForMoving(Card*)));

    PileList fcs;

    for (int i = 0; i < 4; i++)
        if (freecell[i]->isEmpty()) fcs.append(freecell[i]);

    PileList fss;

    for (int i = 0; i < 8; i++)
        if (store[i]->isEmpty() && to != store[i]) fss.append(store[i]);

    if (fcs.count() == 0) {
        assert(fss.count());
        fcs.append(fss.last());
        fss.erase(--fss.end());
    }
    while (moves.count()) { delete moves.first(); moves.erase(moves.begin()); }

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
    kDebug(11111) << debug_level << " movePileToPile" << c.count() << " " << start  << " " << count << endl;
    int moveaway = 0;
    if (count > fcs.count() + 1) {
        moveaway = (fcs.count() + 1);
        while (moveaway * 2 < count)
            moveaway <<= 1;
    }
    kDebug(11111) << debug_level << " moveaway " << moveaway << endl;

    QList<MoveAway> moves_away;

    if (count - moveaway < (fcs.count() + 1) && (count <= 2 * (fcs.count() + 1))) {
        moveaway = count - (fcs.count() + 1);
    }
    while (count > fcs.count() + 1) {
        assert(fss.count());
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
    assert(moving);

    for (int i = 0; i < moving - 1; i++) {
        moves.append(new MoveHint(c[c.count() - i - 1 - start], fcs[i], 0));
    }
    moves.append(new MoveHint(c[c.count() - start - 1 - (moving - 1)], to, 0));

    for (int i = moving - 2; i >= 0; --i)
        moves.append(new MoveHint(c[c.count() - i - 1 - start], to, 0));

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

    MoveHint *mh = moves.first();
    moves.erase(moves.begin());
    CardList empty;
    empty.append(mh->card());
    assert(mh->card() == mh->card()->source()->top());
    assert(mh->pile()->legalAdd(empty));
    mh->pile()->add(mh->card());
    mh->pile()->moveCardsBack(empty, true);
    waitfor = mh->card();
    kDebug(11111) << "wait for moving end " << mh->card()->name() << endl;
    connect(mh->card(), SIGNAL(stoped(Card*)), SLOT(waitForMoving(Card*)));
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
    setWaiting(false);
    c->disconnect(this);
    startMoving();
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
//        kDebug(11111) << "trying " << store->top()->name() << endl;

        CardList cards = store->cards();
        while (cards.count() && !cards.first()->realFace()) cards.erase(cards.begin());

        CardList::Iterator iti = cards.begin();
        while (iti != cards.end())
        {
            if (store->legalRemove(*iti)) {
//                kDebug(11111) << "could remove " << (*iti)->name() << endl;
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
    LocalDealerInfo3() : DealerInfo(I18N_NOOP("&Freecell"), 3) {}
    virtual DealerScene *createGame() { return new Freecell(); }
} ldi8;

//-------------------------------------------------------------------------//

#include "freecell.moc"
