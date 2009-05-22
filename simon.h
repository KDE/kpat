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

#ifndef SIMON_H
#define SIMON_H

#include "dealer.h"


class Simon : public DealerScene {
    Q_OBJECT

    friend class SimonSolver;

public:
    Simon( );

public slots:
    void deal();
    virtual void restart();

protected:
    virtual bool checkAdd   ( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual bool checkPrefering( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual bool checkRemove( int checkIndex, const Pile *c1, const Card *c) const;

private:
    Pile* store[10];
    Pile* target[4];
};

#endif
