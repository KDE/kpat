/*---------------------------------------------------------------------------

  mod3.cpp  implements a patience card game

     Copyright (C) 1997  Rodolfo Borges

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

#include <qdialog.h>
#include "mod3.h"
#include <klocale.h>
#include <kmessagebox.h>
#include "deck.h"
#include "pile.h"
#include <kdebug.h>
#include <kmainwindow.h>
#include <kaction.h>

//-------------------------------------------------------------------------//

void Mod3::restart()
{
    deck->collectAndShuffle();

    deal();
}

//-------------------------------------------------------------------------//

Mod3::Mod3( KMainWindow* parent, const char* _name)
        : Dealer( parent, _name )
{
    deck = new Deck( 0, this, 2);
    deck->hide();

    QValueList<int> moves;
    for (int r = 0; r < 4; r++)
        moves.append(r+1);

    for( int r = 0; r < 4; r++ ) {
        for( int c = 0; c < 8; c++ ) {
            stack[ r ][ c ] = new Pile ( r + 1, this );
            stack[r][c]->move( 8 + 80 * c, 8 + 105 * r + 32 * ( r == 3 ));
            if( r < 3 ) {
                stack[r][c]->setAddFun( &CanPut );
                stack[r][c]->setLegalMove(moves);
            } else
                stack[r][c]->setAddFlags( Pile::addSpread );
        }
    }

    QList<KAction> actions;
    ahint = new KAction( i18n("&Hint"), 0, this,
                         SLOT(hint()),
                         parent->actionCollection(), "game_hint");
    aredeal = new KAction (i18n("&Redeal"), 0, this,
                           SLOT(redeal()),
                           parent->actionCollection(), "game_redeal");
    actions.append(ahint);
    actions.append(aredeal);
    parent->guiFactory()->plugActionList( parent, QString::fromLatin1("game_actions"), actions);

    deal();
}

//-------------------------------------------------------------------------//

bool Mod3::CanPut (const Pile *c1, const CardList &cl)
{
    if (cl.isEmpty())
        return false;

    Card *c2 = cl.first();

    if (c1->isEmpty())
        return (c2->value() == (c1->index()+1));

    if (c1->top()->suit() != c2->suit())
        return false;

    if (c2->value() != (c1->top()->value()+3))
        return false;

    if (c1->cardsLeft() == 1)
        return (c1->top()->value() == (c1->index()+1));

    return true;
}

//-------------------------------------------------------------------------//

void Mod3::redeal()
{
    unmarkAll();

    if (deck->isEmpty()) {
        KMessageBox::information(this, i18n("No more cards"));
        return;
    }
    for (int c = 0; c < 8; c++) {
        Card *card;

        do
            card = deck->nextCard();
        while ((card->value() == Card::Ace) && !deck->isEmpty());

        stack[3][c]->add (card, false, true);
    }

    aredeal->setEnabled( !deck->isEmpty() );
}

//-------------------------------------------------------------------------//

void Mod3::deal()
{
    unmarkAll();

    for (int r = 0; r < 4; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            Card *card;

            do
                card = deck->nextCard();
            while (card->value() == Card::Ace);

            stack[r][c]->add (card, false, true);
        }
    }
    aredeal->setEnabled(true);
}

void Mod3::hint()
{
    unmarkAll();

    PileList candidates;
    for (int r = 0; r < 3; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            Pile *p = stack[r][c];
            if (p->isEmpty()) {
                candidates.append(p);
                continue;
            }
            if (p->cardsLeft() > 1 || p->top()->value() == (p->index()+1))
                candidates.append(p);
        }
    }
    for (int r = 0; r < 4; r++)
    {
        for (int c = 0; c < 8; c++)
        {
            if (stack[r][c]->isEmpty())
                continue;
            CardList empty;
            empty.append(stack[r][c]->top());
            for (PileList::ConstIterator it = candidates.begin();
                 it != candidates.end(); ++it)
            {
                if (CanPut(*it, empty)) {
                    bool markit = true;
                    if (r < 3) { // the card may already be perfect
                        Card *v = stack[r][c]->top();
                        if (v->source()->cardsLeft() > 1 ||
                            v->source()->index() + 1 == v->value())
                            markit = false;
                    }
                    if (markit)
                        mark(empty.first());
                }
            }
        }
    }
}

bool Mod3::isGameWon() const
{
    if (!deck->isEmpty())
        return false;

    for (int c = 0; c < 8; c++)
        if (!stack[3][c]->isEmpty())
            return false;

    // if all cards fit in the top three rows
    // they are correctly set up
    return true;

}

static class LocalDealerInfo7 : public DealerInfo
{
public:
    LocalDealerInfo7() : DealerInfo(I18N_NOOP("M&od3"), 5) {}
    virtual Dealer *createGame(KMainWindow *parent) { return new Mod3(parent); }
} ldi7;

//-------------------------------------------------------------------------//

#include"mod3.moc"

//-------------------------------------------------------------------------//

