/***********************-*-C++-*-********

  idiot.h  implements a patience card game

     Copyright (C) 1995  Paul Olav Tvete <paul@troll.no>

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

****************************************/

#ifndef IDIOT_H
#define IDIOT_H

#include "dealer.h"


class Idiot: public DealerScene
{
    friend class IdiotSolver;

    Q_OBJECT

public:
    Idiot( );
    virtual bool  isGameWon() const;
    virtual void restart();

public slots:
    virtual Card *newCards();

protected:
    virtual bool  cardClicked(Card *);
    virtual bool  cardDblClicked(Card *);

    virtual bool  startAutoDrop()  { return false; }
    virtual void  setGameState(const QString &);

private:
    bool canMoveAway(Card *c) const;

    Pile  *m_play[ 4 ];
    Pile  *m_away;
};

#endif
