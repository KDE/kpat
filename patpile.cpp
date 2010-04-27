/*
 *  Copyright (C) 2010 Parker Coates <parker.coates@kdemail.net>
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

#include "patpile.h"

#include "dealer.h"
#include "renderer.h"


PatPile::PatPile( int index, const QString & objectName )
  : KCardPile(),
    m_index( index ),
    m_role( NoRole ),
    m_foundation( false )
{
    if ( objectName.isEmpty() )
        setObjectName( QString("pile%1").arg( m_index ) );
    else
        setObjectName( objectName );
}


int PatPile::index() const
{
    return m_index;
}


void PatPile::setPileRole( PileRole role )
{
    m_role = role;
}


PatPile::PileRole PatPile::pileRole() const
{
    return m_role;
}


void PatPile::setFoundation( bool foundation )
{
    m_foundation = foundation;
}


bool PatPile::isFoundation() const
{
    return m_foundation;
}


void PatPile::insert( KCard * card, int index )
{
    // FIXME This is hideous and way too casty. Find a more elegant way,
    // maybe moving this code into CardScene to be reimplemented by
    // DealerScene.
    PatPile * oldSource = dynamic_cast<PatPile*>( card->pile() );

    KCardPile::insert( card, index );

    DealerScene * d = dynamic_cast<DealerScene*>( scene() );
    bool takenDown =  d && oldSource && oldSource->isFoundation() && !isFoundation();
    d->preventDropsFor( takenDown, card );
}


void PatPile::paintNormalGraphic( QPainter * painter )
{
    QPixmap pix = Renderer::self()->renderElement( "pile", boundingRect().size().toSize() );
    painter->drawPixmap( 0, 0, pix );
}


void PatPile::paintHighlightedGraphic( QPainter * painter )
{
    QPixmap pix = Renderer::self()->renderElement( "pile_selected", boundingRect().size().toSize() );
    painter->drawPixmap( 0, 0, pix );
}
