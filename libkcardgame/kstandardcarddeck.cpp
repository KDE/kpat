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

#include "kcard.h"


class KStandardCardDeckPrivate
{
};


QList<KStandardCardDeck::Suit> KStandardCardDeck::standardSuits()
{
    return QList<Suit>() << Clubs
                         << Diamonds
                         << Hearts
                         << Spades;
}


QList<KStandardCardDeck::Rank> KStandardCardDeck::standardRanks()
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


quint32 KStandardCardDeck::getId( Suit suit, Rank rank )
{
    return ((suit & 0x3) << 4) | (rank & 0xf);
}


QList<quint32> KStandardCardDeck::generateIdList( int copies,
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
    for ( int i = 0; i < copies; ++i )
        foreach ( const KStandardCardDeck::Suit & s, suits )
            foreach ( const KStandardCardDeck::Rank & r, ranks )
                ids << getId( s, r );

    return ids;
}


KStandardCardDeck::KStandardCardDeck( const KCardTheme & theme, QObject * parent )
  : KAbstractCardDeck( theme, parent ),
    d( new KStandardCardDeckPrivate )
{
}


KStandardCardDeck::~KStandardCardDeck()
{
}


int KStandardCardDeck::rankFromId( quint32 id ) const
{
    int rank = id & 0xf;
    Q_ASSERT( Ace <= rank && rank <= King );
    return rank;
}


int KStandardCardDeck::suitFromId( quint32 id ) const
{
    int suit = (id >> 4) & 0x3;
    Q_ASSERT( Clubs <= suit && suit <= Spades );
    return suit;
}


int KStandardCardDeck::colorFromId( quint32 id ) const
{
    int suit = suitFromId( id );
    return (suit == Clubs || suit == Spades) ? Black : Red;
}


QString KStandardCardDeck::elementName( quint32 id, bool faceUp ) const
{
    if ( !faceUp )
        return "back";

    QString element;

    int rank = id & 0xf;
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

    switch( id >> 4 )
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

