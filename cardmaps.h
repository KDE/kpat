/***********************-*-C++-*-********

  cardmaps.h  defines pixmaps for playing cards

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

#ifndef CARDMAPS_H
#define CARDMAPS_H

#include "card.h"
class cardMapPrivate;

class KConfigGroup;

#include <QtGui/QPixmap>


class cardMap : public QObject
{
    Q_OBJECT

private:
    cardMap();
    ~cardMap();

public:
    static cardMap *self();

    QSize cardSize() const;
    int cardWidth() const;
    int cardHeight() const;
    void setCardWidth( int width );

    QPixmap renderBackside( int variant = -1 );
    QPixmap renderFrontside( Card::Rank, Card::Suit );

    void updateTheme(const KConfigGroup &cg);

public slots:
    void loadInBackground();

private:
    cardMapPrivate *d;

    static cardMap *_self;
};

#endif // KPAT_CARDMAPS_H
