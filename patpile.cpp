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
  : KCardPile( objectName ),
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


QList<KStandardCard*> PatPile::cards() const
{
    return castCardList( KCardPile::cards() );
}


KStandardCard * PatPile::at( int index ) const
{
    return static_cast<KStandardCard*>( KCardPile::at( index ) );
}


KStandardCard* PatPile::top() const
{
    return static_cast<KStandardCard*>( KCardPile::top() );
}


QList< KStandardCard* > PatPile::topCardsDownTo( const KCard * card ) const
{
    return castCardList( KCardPile::topCardsDownTo( card ) );
}


void PatPile::add( KCard * card, int index )
{
    Q_ASSERT( dynamic_cast<KStandardCard*>( card ) );

    PatPile * oldSource = dynamic_cast<PatPile*>( card->source() );

    KCardPile::add( card, index );

    KStandardCard * c = dynamic_cast<KStandardCard*>( card );
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
