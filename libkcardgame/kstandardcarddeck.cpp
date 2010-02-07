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


class KStandardCardDeckPrivate
{
public:
    QList<quint32> generateIds( int copies, QList<KStandardCardDeck::Suit> suits, QList<KStandardCardDeck::Rank> ranks )
    {
        Q_ASSERT( copies >= 1 );
        Q_ASSERT( suits.size() >= 1 );
        Q_ASSERT( ranks.size() >= 1 );

        // Note the order the cards are created in can't be changed as doing so
        // will mess up the game numbering.
        QList<quint32> ids;
        for ( int i = 0; i < copies; ++i )
            foreach ( const KStandardCardDeck::Rank & r, ranks )
                foreach ( const KStandardCardDeck::Suit & s, suits )
                    ids << ( s << 4 ) + r;

        Q_ASSERT( ids.size() == copies * ranks.size() * suits.size() );

        return ids;
    }
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


KStandardCardDeck::KStandardCardDeck( int copies, QList<Suit> suits, QList<Rank> ranks )
  : KAbstractCardDeck( d->generateIds( copies, suits, ranks ) ),
    d( new KStandardCardDeckPrivate )
{
    foreach ( KCard * c, cards() )
        c->setObjectName( elementName( c->data() ) );
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


KStandardCardDeck::Suit getSuit( const KCard * card )
{
    KStandardCardDeck::Suit s = KStandardCardDeck::Suit( card->data() >> 4 );
    Q_ASSERT( KStandardCardDeck::Clubs <= s && s <= KStandardCardDeck::Spades );
    return s;
}


KStandardCardDeck::Rank getRank( const KCard * card )
{
    KStandardCardDeck::Rank r = KStandardCardDeck::Rank( card->data() & 0xf );
    Q_ASSERT( KStandardCardDeck::Ace <= r && r <= KStandardCardDeck::King );
    return r;
}


bool getIsRed( const KCard * card )
{
    KStandardCardDeck::Suit s = getSuit( card );
    return s == KStandardCardDeck::Hearts || s == KStandardCardDeck::Diamonds;
}

