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
    KCardPile * pile;
    int index;
    bool faceUp;
    bool takenDown;

    CardState()
      : pile( 0 ),
        index( -1 ),
        faceUp( false ),
        takenDown( false )
    {
    }
    
    CardState( KCardPile * pile, int index, bool faceUp, bool takenDown )
      : pile( pile ),
        index( index ),
        faceUp( faceUp ),
        takenDown( takenDown )
    {
    }

    bool operator==( const CardState & rhs ) const
    {
        return pile == rhs.pile
               && index == rhs.index
               && faceUp == rhs.faceUp
               && takenDown == rhs.takenDown;
    }

    bool operator!=( const CardState & rhs ) const
    {
        return !operator==( rhs );
    }
};


class CardStateChange
{
public:
    CardState oldState;
    CardState newState;
    QList<KCard*> cards;

    CardStateChange( CardState oldState, CardState newState, QList<KCard*> cards )
      : oldState( oldState ),
        newState( newState ),
        cards( cards )
    {
    };
};

class GameState
{
public:
    QList<CardStateChange> changes;
    QString stateData;
    Solver::ExitStatus solvability;
    QList<MOVE> winningMoves;

    GameState( QList<CardStateChange> changes, QString stateData )
      : changes ( changes ),
        stateData( stateData ),
        solvability( Solver::SearchAborted )
    {
    }
};


#endif
