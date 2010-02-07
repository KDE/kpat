/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
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

#include "kstandardcard.h"


KStandardCard::KStandardCard( Rank r, Suit s, KAbstractCardDeck * deck )
  : KCard( (s << 4) + r, deck ),
    m_takenDown( false )
{
    Q_ASSERT( deck );

    QString suitName;
    switch( suit() )
    {
        case Clubs :    suitName = "Clubs";    break;
        case Diamonds : suitName = "Diamonds"; break;
        case Hearts :   suitName = "Hearts";   break;
        case Spades :   suitName = "Spades";   break;
        default :       suitName = "???";      break;
    }
    setObjectName( suitName + QString::number( rank() ) );
}


KStandardCard::~KStandardCard()
{
}


KStandardCard::Suit KStandardCard::suit() const
{
    return getSuit( this );
}


KStandardCard::Rank KStandardCard::rank() const
{
    return getRank( this );
}


bool KStandardCard::isRed() const
{
    return suit() == Diamonds || suit() == Hearts;
}


void KStandardCard::setTakenDown(bool td)
{
    m_takenDown = td;
}


bool KStandardCard::takenDown() const
{
    return m_takenDown;
}


KStandardCard::Suit getSuit( const KCard * card )
{
    KStandardCard::Suit s = KStandardCard::Suit( card->data() >> 4 );
    Q_ASSERT( KStandardCard::Clubs <= s && s <= KStandardCard::Spades );
    return s;
}


KStandardCard::Rank getRank( const KCard * card )
{
    KStandardCard::Rank r = KStandardCard::Rank( card->data() & 0xf );
    Q_ASSERT( KStandardCard::Ace <= r && r <= KStandardCard::King );
    return r;
}


QList<KStandardCard*> castCardList( const QList<KCard*> & cards )
{
    QList<KStandardCard*> result;
    foreach ( KCard * c, cards )
    {
        Q_ASSERT( dynamic_cast<KStandardCard*>( c ) );
        result << static_cast<KStandardCard*>( c );
    }
    return result;
}

