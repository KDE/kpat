/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009 Parker Coates <parker.coates@gmail.com>
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

#include "cardscene.h"

#include "carddeck.h"
#include "pile.h"


CardScene::CardScene( QObject * parent )
  : PatienceGraphicsScene( parent ),
    m_deck( 0 )
{
}


CardScene::~CardScene()
{
    delete m_deck;

    foreach ( Pile * p, m_piles )
    {
        removePile( p );
        delete p;
    }
    m_piles.clear();

    Q_ASSERT( items().isEmpty() );
}


void CardScene::setDeck( CardDeck * deck )
{
    delete m_deck;
    m_deck = deck;
}


CardDeck * CardScene::deck() const
{
    return m_deck;
}


void CardScene::addPile( Pile * pile )
{
    if ( pile->dscene() )
        pile->dscene()->removePile( pile );

    addItem( pile );
    foreach ( Card * c, pile->cards() )
        addItem( c );
    m_piles.append( pile );
}


void CardScene::removePile( Pile * pile )
{
    foreach ( Card * c, pile->cards() )
        removeItem( c );
    removeItem( pile );
    m_piles.removeAll( pile );
}


QList<Pile*> CardScene::piles() const
{
    return m_piles;
}







