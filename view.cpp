/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2008 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009 Parker Coates <parker.coates@kde.org>
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
#include "dealer.h"
#include "deck.h"

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
        Deck::self()->setCardWidth( int( Deck::self()->cardWidth() / scaleFactor ) );
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
