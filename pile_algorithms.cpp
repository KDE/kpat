/*
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
*/
#include "pile.h"

#include <kdebug.h>

bool Pile::add_klondikeTarget( const CardList& c2 ) const
{
    Card *newone = c2.first();
    if (isEmpty())
        return (newone->rank() == Card::Ace);

    return (newone->rank() == top()->rank() + 1)
          && (top()->suit() == newone->suit());
}

bool Pile::add_klondikeStore(  const CardList& c2 ) const
{
    Card *newone = c2.first();
    if (isEmpty()) {
        return (newone->rank() == Card::King);
    }

    return (newone->rank() == top()->rank() - 1)
        && (top()->isRed() != newone->isRed());
}

bool Pile::add_gypsyStore( const CardList& c2) const
{
    Card *newone = c2.first();
    if (isEmpty())
        return true;

    return (newone->rank() == top()->rank() - 1)
             && (top()->isRed() != newone->isRed());
}

bool Pile::add_freeCell( const CardList & cards) const
{
    return (cards.count() == 1 && isEmpty());
}

bool Pile::remove_freecellStore( const Card *c) const
{
    // ok if just one card
    if (c == top())
        return true;

    // Now we're trying to move two or more cards.

    // First, let's check if the column is in valid
    // (that is, in sequence, alternated colors).
    int index = indexOf(c) + 1;
    const Card *before = c;
    while (true)
    {
        c = at(index++);

        if (!((c->rank() == (before->rank()-1))
              && (c->isRed() != before->isRed())))
        {
            kDebug(11111) << c->rank() << " " << c->suit() << " - " << before->rank() << " " << before->suit();
            return false;
        }
        if (c == top())
            return true;
        before = c;
    }

    return true;
}

