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

#ifndef KSTANDARDCARD_H
#define KSTANDARDCARD_H

#include "kcard.h"
#include "libkcardgame_export.h"
class KCardPile;

#include <QtCore/QList>


class LIBKCARDGAME_EXPORT KStandardCard: public KCard
{
    friend class KAbstractCardDeck;

public:
    enum Suit
    {
        NoSuit = -1,
        Clubs = 0,
        Diamonds = 1,
        Hearts = 2,
        Spades = 3
    };

    enum Rank
    {
        NoRank = 0,
        Ace = 1,
        Two,
        Three,
        Four,
        Five,
        Six,
        Seven,
        Eight,
        Nine,
        Ten,
        Jack,
        Queen,
        King
    };

    KStandardCard( Rank r, Suit s, KAbstractCardDeck * deck );
    virtual ~KStandardCard();

    Suit suit() const;
    Rank rank() const;
    bool isRed() const;
};

LIBKCARDGAME_EXPORT KStandardCard::Suit getSuit( const KCard * card );
LIBKCARDGAME_EXPORT KStandardCard::Rank getRank( const KCard * card );
LIBKCARDGAME_EXPORT QList<KStandardCard*> castCardList( const QList<KCard*> & cards );

#endif
