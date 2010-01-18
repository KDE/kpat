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

#include "render.h"


PatPile::PatPile( int index, const QString & objectName )
  : Pile( objectName ),
    m_index( index ),
    m_role( NoRole ),
    m_foundation( false )
{
    if ( objectName.isEmpty() )
        setObjectName( QString("pile%1").arg( m_index ) );
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


QList<StandardCard*> PatPile::cards() const
{
    return castCardList( Pile::cards() );
}


StandardCard * PatPile::at( int index ) const
{
    return static_cast<StandardCard*>( Pile::at( index ) );
}


StandardCard* PatPile::top() const
{
    return static_cast<StandardCard*>( Pile::top() );
}


QList< StandardCard* > PatPile::topCardsDownTo( const Card * card ) const
{
    return castCardList( Pile::topCardsDownTo( card ) );
}


void PatPile::add( Card * card, int index )
{
    Q_ASSERT( dynamic_cast<StandardCard*>( card ) );

    PatPile * oldSource = dynamic_cast<PatPile*>( card->source() );

    Pile::add( card, index );

    StandardCard * c = dynamic_cast<StandardCard*>( card );
    if ( c )
        c->setTakenDown( oldSource && oldSource->isFoundation() && !isFoundation() );
}


QPixmap PatPile::normalPixmap( QSize size )
{
    return Render::renderElement( "pile", size );
}


QPixmap PatPile::highlightedPixmap( QSize size )
{
    return Render::renderElement( "pile_selected", size );
}
