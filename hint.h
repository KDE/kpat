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
#ifndef HINT_H
#define HINT_H

#include <QList>

class Card;
class Pile;


class MoveHint
{
public:
    MoveHint(Card *card, Pile *to, int prio);

    Card  *card() const         { return m_card;         }
    Pile  *pile() const         { return m_to;           }
    int priority() const      { return m_prio; }

private:
    Card  *m_card;
    Pile  *m_to;
    int m_prio;
};


typedef QList<MoveHint*>  HintList;


#endif
