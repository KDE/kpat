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


PatPile::PatPile( KCardScene * cardScene, int index, const QString & objectName )
  : KCardPile( cardScene ),
    m_index( index ),
    m_role( NoRole )
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


bool PatPile::isFoundation() const
{
    return FoundationType1 <= m_role && m_role <= FoundationType4;
}


void PatPile::paintGraphic( QPainter * painter, qreal highlightedness )
{
    const QSize size = boundingRect().size().toSize();
    Renderer * r = Renderer::self();

    if ( highlightedness < 1 )
        painter->drawPixmap( 0, 0, r->renderElement( "pile", size ) );

    if ( highlightedness > 0 )
    {
        if ( highlightedness < 1 )
        {
            // Using QPainter::setOpacity is currently very inefficient, so to
            // paint a semitransparent pixmap, we have to do some fiddling.
            QPixmap transPix( size );
            transPix.fill( Qt::transparent );
            QPainter p( &transPix );
            p.drawPixmap( 0, 0, r->renderElement( "pile_selected", size ) );
            p.setCompositionMode( QPainter::CompositionMode_DestinationIn );
            p.fillRect( transPix.rect(), QColor( 0, 0, 0, highlightedness * 255 ) );
            painter->drawPixmap( 0, 0, transPix );
        }
        else
        {
            painter->drawPixmap( 0, 0, r->renderElement( "pile_selected", size ) );
        }
    }
}

