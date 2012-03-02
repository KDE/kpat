/*
 *  Copyright (C) 2010 Parker Coates <coates@kde.org>
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

#include "kcarddeck.h"

#include "kcard.h"


class KStandardCardDeckPrivate
{
};


QList<KCardDeck::Suit> KCardDeck::standardSuits()
{
    return QList<Suit>() << Clubs
                         << Diamonds
                         << Hearts
                         << Spades;
}


QList<KCardDeck::Rank> KCardDeck::standardRanks()
{
    return QList<Rank>() << Ace
                         << Two
                         << Three
                         << Four
                         << Five
                         << Six
                         << Seven
                         << Eight
                         << Nine
                         << Ten
                         << Jack
                         << Queen
                         << King;
}


quint32 KCardDeck::getId( KCardDeck::Suit suit, KCardDeck::Rank rank, int number )
{
    return (number << 16) | ((suit & 0xff) << 8) | (rank & 0xff);
}


QList<quint32> KCardDeck::generateIdList( int copies,
                                          const QList<Suit> & suits,
                                          const QList<Rank> & ranks )
{
    Q_ASSERT( copies >= 1 );
    Q_ASSERT( !suits.isEmpty() );
    Q_ASSERT( !ranks.isEmpty() );

    // Note that changing the order in which the cards are created should be
    // avoided if at all possible. Doing so may effect the game logic of
    // games relying on LibKCardGame.
    QList<quint32> ids;
    unsigned int number = 0;
    for ( int c = 0; c < copies; ++c )
        foreach ( const Suit & s, suits )
            foreach ( const Rank & r, ranks )
                ids << getId( s, r, number++ );

    return ids;
}


KCardDeck::KCardDeck( const KCardTheme & theme, QObject * parent )
  : KAbstractCardDeck( theme, parent ),
    d( new KStandardCardDeckPrivate )
{
}


KCardDeck::~KCardDeck()
{
}


int KCardDeck::rankFromId( quint32 id ) const
{
    int rank = id & 0xff;
    Q_ASSERT( Ace <= rank && rank <= King );
    return rank;
}


int KCardDeck::suitFromId( quint32 id ) const
{
    int suit = (id >> 8) & 0xff;
    Q_ASSERT( Clubs <= suit && suit <= Spades );
    return suit;
}


int KCardDeck::colorFromId( quint32 id ) const
{
    int suit = suitFromId( id );
    return (suit == Clubs || suit == Spades) ? Black : Red;
}


QString KCardDeck::elementName( quint32 id, bool faceUp ) const
{
    if ( !faceUp )
        return "back";

    QString element;

    int rank = rankFromId( id );
    switch( rank )
    {
    case King:
        element = "king";
        break;
    case Queen:
        element = "queen";
        break;
    case Jack:
        element = "jack";
        break;
    default:
        element = QString::number( rank );
        break;
    }

    switch( suitFromId( id ) )
    {
    case Clubs:
        element += "_club";
        break;
    case Spades:
        element += "_spade";
        break;
    case Diamonds:
        element += "_diamond";
        break;
    case Hearts:
        element += "_heart";
        break;
    }

    return element;
}

