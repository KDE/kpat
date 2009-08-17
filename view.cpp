/*
    patience -- main program

    Copyright (C) 1995  Paul Olav Tvete <paul@troll.no>
    Copyright (C) 2007 Simon Hürlimann <simon.huerlimann@huerlisi.ch>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "view.h"
#include "dealer.h"
#include "cardmaps.h"

#include <cmath>

#include <QtGui/QResizeEvent>
#include <QtGui/QWheelEvent>

#include <kdebug.h>

// ================================================================
//                        class PatienceView


PatienceView::PatienceView( QWidget* _parent )
  : QGraphicsView( _parent )
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(QFrame::NoFrame);
    setAlignment( Qt::AlignLeft | Qt::AlignTop );
    setCacheMode(QGraphicsView::CacheBackground);

#ifndef QT_NO_OPENGL
    //QGLWidget *wgl = new QGLWidget();
    //setupViewport(wgl);
#endif
}

PatienceView::~PatienceView()
{
}

void PatienceView::setScene( QGraphicsScene *_scene )
{
    QGraphicsView::setScene( _scene );
    DealerScene* dealer = dynamic_cast<DealerScene*>( _scene );
    if ( dealer )
    {
        resetCachedContent();
        dealer->setSceneSize( size() );
    }
}

void PatienceView::wheelEvent( QWheelEvent *e )
{
    if ( e->modifiers() & Qt::ControlModifier )
    {
        qreal scaleFactor = pow((qreal)2, -e->delta() / (10*120.0));
        cardMap::self()->setCardWidth( int( cardMap::self()->cardWidth() / scaleFactor ) );
    }
}

void PatienceView::resizeEvent( QResizeEvent *e )
{
    QGraphicsView::resizeEvent(e);

    DealerScene* dealer = dynamic_cast<DealerScene*>( scene() );
    if ( dealer )
    {
        resetCachedContent();
        dealer->setSceneSize( size() );
    }
}

#include "view.moc"
