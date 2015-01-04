/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2008 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009-2010 Parker Coates <coates@kde.org>
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

#include "view.h"

#include "gameselectionscene.h"
#include "renderer.h"

#include "KCardScene"

#include <QResizeEvent>


// ================================================================
//                        class PatienceView


PatienceView::PatienceView( QWidget * parent )
  : QGraphicsView( parent ),
    KGameRendererClient( Renderer::self(), "background" )
{
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setFrameStyle( QFrame::NoFrame );

    // This makes the background of the widget transparent so that Oxygen's
    // (or any other style's) window gradient is visible in unpainted areas of
    // the scene.
    QPalette p = palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );
    setPalette( p );
    setBackgroundRole( QPalette::Base );

    setAlignment( Qt::AlignLeft | Qt::AlignTop );
    setCacheMode( QGraphicsView::CacheBackground );
}


PatienceView::~PatienceView()
{
}


void PatienceView::setScene( QGraphicsScene * scene )
{
    QGraphicsView::setScene( scene );
    updateSceneSize();
}


void PatienceView::resizeEvent( QResizeEvent * e )
{
    QGraphicsView::resizeEvent( e );
    setRenderSize( e->size() );
    resetCachedContent();
    updateSceneSize();
}


void PatienceView::drawBackground( QPainter * painter, const QRectF & rect )
{
    QRectF source = rect.translated( -sceneRect().topLeft() );
    if ( m_background.size() != size() )
    {
        qreal xScale = m_background.width() / width();
        qreal yScale = m_background.height() / height();
        source = QRectF( source.x() * xScale,
                         source.y() * yScale,
                         source.width() * xScale,
                         source.height() * yScale );
    }

    painter->drawPixmap( rect, m_background, source );
}


void PatienceView::receivePixmap( const QPixmap & pixmap )
{
    m_background = pixmap;
    resetCachedContent();
}


void PatienceView::updateSceneSize()
{
    KCardScene * cs = dynamic_cast<KCardScene*>( scene() );
    if ( cs )
    {
        cs->resizeScene( size() );
    }
    else
    {
        GameSelectionScene * gss = dynamic_cast<GameSelectionScene*>( scene() );
        if ( gss )
            gss->resizeScene( size() );
    }
}
