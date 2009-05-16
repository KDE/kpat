/***********************-*-C++-*-********

  cardmaps.h  defines pixmaps for playing cards

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

****************************************/

#ifndef CARDMAPS_H
#define CARDMAPS_H

#include "card.h"

#include <QPixmap>

class cardMapPrivate;
class KConfigGroup;

class cardMap : public QObject
{
    Q_OBJECT
public:

    static cardMap *self();
    cardMap();
    ~cardMap();

    QSize cardSize() const;
    int cardWidth() const;
    int cardHeight() const;
    void setCardWidth( int width );

    QPixmap renderBackside( int variant = -1 );
    QPixmap renderFrontside( Card::Rank, Card::Suit );
    QRect opaqueRect() const;

    void updateTheme(const KConfigGroup &cg);

public slots:
    void triggerRescale();

private:
    cardMapPrivate *d;

    static cardMap *_self;
};

#endif // KPAT_CARDMAPS_H
