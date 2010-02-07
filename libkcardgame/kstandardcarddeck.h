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

#ifndef KSTANDARDCARDDECK_H
#define KSTANDARDCARDDECK_H

#include "carddeck.h"
#include "standardcard.h"
#include "libkcardgame_export.h"


class LIBKCARDGAME_EXPORT KStandardCardDeck : public CardDeck
{
public:

    static QList<StandardCard::Suit> standardSuits();
    static QList<StandardCard::Rank> standardRanks();

    explicit KStandardCardDeck( int copies = 1,
                                QList<StandardCard::Suit> suits = standardSuits(),
                                QList<StandardCard::Rank> ranks = standardRanks() );
    virtual ~KStandardCardDeck();

protected:
    virtual QString elementName(quint32 id, bool faceUp = true) const;
};

#endif
