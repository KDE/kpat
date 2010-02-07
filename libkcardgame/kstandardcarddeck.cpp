/*
 *  Copyright (C) 2010 Parker Coates <parker.coates@kdemail.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of 
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "kstandardcarddeck.h"


QList<StandardCard::Suit> KStandardCardDeck::standardSuits()
{
    return QList<StandardCard::Suit>() << StandardCard::Clubs
                                       << StandardCard::Diamonds
                                       << StandardCard::Hearts
                                       << StandardCard::Spades;
}


QList<StandardCard::Rank> KStandardCardDeck::standardRanks()
{
    return QList<StandardCard::Rank>() << StandardCard::Ace
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
                                       << StandardCard::King;
}



KStandardCardDeck::KStandardCardDeck( int copies, QList<StandardCard::Suit> suits, QList<StandardCard::Rank> ranks )
  : KAbstractCardDeck()
{
    Q_ASSERT( copies >= 1 );
    Q_ASSERT( suits.size() >= 1 );
    Q_ASSERT( ranks.size() >= 1 );

    // Note the order the cards are created in can't be changed as doing so
    // will mess up the game numbering.
    QList<Card*> cards;
    for ( int i = 0; i < copies; ++i )
        foreach ( const StandardCard::Rank r, ranks )
            foreach ( const StandardCard::Suit s, suits )
                cards << new StandardCard( r, s, this );

    Q_ASSERT( cards.size() == copies * ranks.size() * suits.size() );

    initializeCards( cards );
}


KStandardCardDeck::~KStandardCardDeck()
{
}


QString KStandardCardDeck::elementName( quint32 id, bool faceUp ) const
{
    if ( !faceUp )
        return "back";

    QString element;

    int rank = id & 0xf;
    switch( rank )
    {
    case StandardCard::King:
        element = "king";
        break;
    case StandardCard::Queen:
        element = "queen";
        break;
    case StandardCard::Jack:
        element = "jack";
        break;
    default:
        element = QString::number( rank );
        break;
    }

    switch( id >> 4 )
    {
    case StandardCard::Clubs:
        element += "_club";
        break;
    case StandardCard::Spades:
        element += "_spade";
        break;
    case StandardCard::Diamonds:
        element += "_diamond";
        break;
    case StandardCard::Hearts:
        element += "_heart";
        break;
    }

    return element;
}

