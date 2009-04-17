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

#ifndef GYPSY_H
#define GYPSY_H

#include "dealer.h"

class Pile;

class Gypsy : public DealerScene
{
    friend class GypsySolver;
    Q_OBJECT

public:
    Gypsy( );
    virtual void restart();

public slots:
    virtual Card *newCards();

protected:
    virtual void setGameState(const QString &);

private: // functions
    void deal();
    void dealRow(bool faceup);

private:
    Pile* store[8];
    Pile* target[8];
};

#endif
