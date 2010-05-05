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

#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "patsolve/patsolve.h"

class KCard;
class KCardPile;

#include <QtCore/QSet>
#include <QtCore/QString>


class CardState
{
public:
    KCard * card;
    KCardPile * pile;
    bool faceUp;
    bool takenDown;
    int cardIndex;

    bool operator==( const CardState & rhs ) const
    {
        return card == rhs.card
               && pile == rhs.pile
               && faceUp == rhs.faceUp
               && cardIndex == rhs.cardIndex
               && takenDown == rhs.takenDown;
    }
};


uint qHash( const CardState & state )
{
    return qHash( state.card );
}


class GameState
{
public:
    QSet<CardState> cards;
    QString gameData;
    Solver::ExitStatus solvability;
    QList<MOVE> winningMoves;

    GameState( QSet<CardState> cardStates, QString gameData )
      : cards( cardStates ),
        gameData( gameData ),
        solvability( Solver::SearchAborted )
    {
    }

    bool operator==( const GameState & rhs ) const
    {
        return cards == rhs.cards && gameData == rhs.gameData;
    }
};


#endif
