/*
 *  Copyright (C) 2010 Parker Coates <coates@kde.org>
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


PatPile::PatPile( DealerScene * scene, int index, const QString & objectName )
  : KCardPile( scene ),
    m_index( index ),
    m_role( NoRole )
{
    if ( objectName.isEmpty() )
        setObjectName( QString(QLatin1String("pile%1" )).arg( m_index ) );
    else
        setObjectName( objectName );

    // Set the default spread for all piles in KPat.
    setSpread( 0, 0.33 );

    if ( scene )
        scene->addPatPile( this );
}


PatPile::~PatPile()
{
    DealerScene * dealerScene = dynamic_cast<DealerScene*>( scene() );
    if ( dealerScene )
        dealerScene->removePatPile( this );
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


QList< QPointF > PatPile::cardPositions() const
{
    QList<QPointF> positions;
    QPointF currentPosition( 0, 0 );
    foreach( KCard * c, cards() )
    {
        positions << currentPosition;
        qreal adjustment = c->isFaceUp() ? 1 : 0.6;
        currentPosition += adjustment * spread();
    }
    return positions;
}


void PatPile::paintGraphic( QPainter * painter, qreal highlightedness )
{
    const QSize size = boundingRect().size().toSize();
    Renderer * r = Renderer::self();

    if ( highlightedness < 1 )
        painter->drawPixmap( 0, 0, r->spritePixmap( "pile", size ) );

    if ( highlightedness > 0 )
    {
        if ( highlightedness < 1 )
        {
            // Using QPainter::setOpacity is currently very inefficient, so to
            // paint a semitransparent pixmap, we have to do some fiddling.
            QPixmap transPix( size );
            transPix.fill( Qt::transparent );
            QPainter p( &transPix );
            p.drawPixmap( 0, 0, r->spritePixmap( "pile_selected", size ) );
            p.setCompositionMode( QPainter::CompositionMode_DestinationIn );
            p.fillRect( transPix.rect(), QColor( 0, 0, 0, highlightedness * 255 ) );
            painter->drawPixmap( 0, 0, transPix );
        }
        else
        {
            painter->drawPixmap( 0, 0, r->spritePixmap( "pile_selected", size ) );
        }
    }
}

