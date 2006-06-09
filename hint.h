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
//Added by qt3to4:
#include <QList>
#ifndef HINT_H
#define HINT_H


class Card;
class Pile;


class MoveHint
{
public:
    MoveHint(Card *card, Pile *to, bool d=true);

    bool   dropIfTarget() const { return m_dropiftarget; }
    Card  *card() const         { return m_card;         }
    Pile  *pile() const         { return m_to;           }

private:
    Card  *m_card;
    Pile  *m_to;
    bool   m_dropiftarget;
};


typedef QList<MoveHint*>  HintList;


#endif
