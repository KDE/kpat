/***********************-*-C++-*-********

  grandf.cpp  implements a patience card game

     Copyright (C) 1995  Paul Olav Tvete

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


//
// 7 positions, all cards on table, follow suit
// ( I don't know a name for this one, but I learned it from my grandfather.)

****************************************/


#ifndef P_GRANDF_7
#define P_GRANDF_7

#include "dealer.h"

class KAction;
class Pile;
class Deck;
class KMainWindow;

class Grandf : public Dealer {
    Q_OBJECT

public:
    Grandf( KMainWindow* parent=0, const char* name=0);

public slots:
    void redeal();
    void deal();
    virtual void restart();
    virtual bool isGameLost() const;


protected:
    void collect();
    virtual bool checkAdd   ( int checkIndex, const Pile *c1, const CardList& c2) const;
    virtual QString getGameState() const;
    virtual void setGameState( const QString & stream );
    virtual Card *demoNewCards();

private:
    Pile* store[7];
    Pile* target[4];
    Deck *deck;
    int numberOfDeals;

};

#endif
