/***********************-*-C++-*-********

  klondike.cpp  implements a patience card game

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
// 7 positions, alternating red and black
//


****************************************/

#ifndef KLONDIKE_H
#define KLONDIKE_H

#include "dealer.h"
class KlondikePile;

class KSelectAction;


class Klondike : public DealerScene {
    friend class KlondikeSolver;

    Q_OBJECT

public:
    explicit Klondike();

    virtual void mapOldId(int id);
    virtual void restart();
    virtual bool startAutoDrop();

    void redeal(); // put pile back into deck
    void deal();

    virtual bool cardClicked(Card *);

    void setEasy( bool easy );

public slots:
    virtual Card *newCards();

protected:
    virtual QString getGameState();
    virtual QString getGameOptions() const;
    virtual void setGameOptions(const QString &options);

private:
    bool EasyRules;
    bool redealt;

    KSelectAction *options;

    Pile* play[7];
    Pile* target[4];

    KlondikePile *pile;
    Card::Rank target_tops[4];
    bool noLongerNeeded(Card::Rank r, Card::Suit s );

private slots:
    void gameTypeChanged();
};

class KlondikePile : public Pile
{
public:
    KlondikePile( int _index, int _draw, DealerScene* parent);
    int draw() const { return m_draw; }
    virtual QSizeF cardOffset( const Card * ) const;
    virtual void relayoutCards();
    void setDraws( int _draw );

private:
    int m_draw;
};

#endif
