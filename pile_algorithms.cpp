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
            kdDebug(11111) << c->name() << " - " << before->name() << endl;
            return false;
        }
        if (c == top())
            return true;
        before = c;
    }

    return true;
}

