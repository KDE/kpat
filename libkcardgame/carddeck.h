/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009 Parker Coates <parker.coates@kdemail.net>
 *
 * License of original code:
 * -------------------------------------------------------------------------
 *   Permission to use, copy, modify, and distribute this software and its
 *   documentation for any purpose and without fee is hereby granted,
 *   provided that the above copyright notice appear in all copies and that
 *   both that copyright notice and this permission notice appear in
 *   supporting documentation.
 *
 *   This file is provided AS IS with no warranties of any kind.  The author
 *   shall have no liability with respect to the infringement of copyrights,
 *   trade secrets or any patents by this file or any part thereof.  In no
 *   event will the author be liable for any lost revenue or profits or
 *   other special, indirect and consequential damages.
 * -------------------------------------------------------------------------
 *
 * License of modifications/additions made after 2009-01-01:
 * -------------------------------------------------------------------------
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of 
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * -------------------------------------------------------------------------
 */

#ifndef CARDDECK_H
#define CARDDECK_H

#include "standardcard.h"
#include "libkcardgame_export.h"
class Pile;

#include "kcardcache.h"
class KConfigGroup;

#include <QSet>


class LIBKCARDGAME_EXPORT CardDeck : public QObject
{
    Q_OBJECT

public:
    explicit CardDeck( int copies = 1,
                       QList<StandardCard::Suit> suits = QList<StandardCard::Suit>()
                                                         << StandardCard::Clubs
                                                         << StandardCard::Diamonds
                                                         << StandardCard::Hearts
                                                         << StandardCard::Spades,
                       QList<StandardCard::Rank> ranks = QList<StandardCard::Rank>()
                                                         << StandardCard::Ace
                                                         << StandardCard::Two
                                                         << StandardCard::Three
                                                         << StandardCard::Four
                                                         << StandardCard::Five
                                                         << StandardCard::Six
                                                         << StandardCard::Seven
                                                         << StandardCard::Eight
                                                         << StandardCard::Nine
                                                         << StandardCard::Ten
                                                         << StandardCard::Jack
                                                         << StandardCard::Queen
                                                         << StandardCard::King
                     );
    virtual ~CardDeck();

    QList<Card*> cards() const;

    bool hasUndealtCards() const;
    Card * takeCard();
    Card * takeCard( quint32 id );
    void takeAllCards( Pile * p );
    void returnCard( Card * c );
    void returnAllCards();
    void shuffle( int gameNumber );

    void setCardWidth( int width );
    int cardWidth() const;
    void setCardHeight( int height );
    int cardHeight() const;
    QSize cardSize() const;

    QPixmap frontsidePixmap( quint32 id ) const;
    QPixmap backsidePixmap( quint32 id ) const;

    void updateTheme( const KConfigGroup & cg );

    bool hasAnimatedCards() const;

signals:
    void cardAnimationDone();

public slots:
    void loadInBackground();

protected:
    virtual QString elementName( quint32 id, bool faceUp = true ) const;

private slots:
    void cardStartedAnimation( Card * card );
    void cardStoppedAnimation( Card * card );

private:
    QList<Card*> m_allCards;
    QList<Card*> m_undealtCards;

    KCardCache2 * m_cache;
    QSizeF m_originalCardSize;
    QSize m_currentCardSize;

    QSet<Card*> m_cardsWaitedFor;
};

#endif
