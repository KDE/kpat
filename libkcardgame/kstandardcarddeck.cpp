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


QList<KStandardCard::Suit> KStandardCardDeck::standardSuits()
{
    return QList<KStandardCard::Suit>() << KStandardCard::Clubs
                                       << KStandardCard::Diamonds
                                       << KStandardCard::Hearts
                                       << KStandardCard::Spades;
}


QList<KStandardCard::Rank> KStandardCardDeck::standardRanks()
{
    return QList<KStandardCard::Rank>() << KStandardCard::Ace
                                       << KStandardCard::Two
                                       << KStandardCard::Three
                                       << KStandardCard::Four
                                       << KStandardCard::Five
                                       << KStandardCard::Six
                                       << KStandardCard::Seven
                                       << KStandardCard::Eight
                                       << KStandardCard::Nine
                                       << KStandardCard::Ten
                                       << KStandardCard::Jack
                                       << KStandardCard::Queen
                                       << KStandardCard::King;
}



KStandardCardDeck::KStandardCardDeck( int copies, QList<KStandardCard::Suit> suits, QList<KStandardCard::Rank> ranks )
  : KAbstractCardDeck()
{
    Q_ASSERT( copies >= 1 );
    Q_ASSERT( suits.size() >= 1 );
    Q_ASSERT( ranks.size() >= 1 );

    // Note the order the cards are created in can't be changed as doing so
    // will mess up the game numbering.
    QList<KCard*> cards;
    for ( int i = 0; i < copies; ++i )
        foreach ( const KStandardCard::Rank r, ranks )
            foreach ( const KStandardCard::Suit s, suits )
                cards << new KStandardCard( r, s, this );

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
    case KStandardCard::King:
        element = "king";
        break;
    case KStandardCard::Queen:
        element = "queen";
        break;
    case KStandardCard::Jack:
        element = "jack";
        break;
    default:
        element = QString::number( rank );
        break;
    }

    switch( id >> 4 )
    {
    case KStandardCard::Clubs:
        element += "_club";
        break;
    case KStandardCard::Spades:
        element += "_spade";
        break;
    case KStandardCard::Diamonds:
        element += "_diamond";
        break;
    case KStandardCard::Hearts:
        element += "_heart";
        break;
    }

    return element;
}

