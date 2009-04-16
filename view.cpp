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


PatienceView *PatienceView::s_instance = 0;

PatienceView::PatienceView( KXmlGuiWindow* _window, QWidget* _parent )
  : QGraphicsView( _parent ),
    m_window( _window )
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(QFrame::NoFrame);
    setAlignment( Qt::AlignLeft | Qt::AlignTop );
    setCacheMode(QGraphicsView::CacheBackground);

    Q_ASSERT(!s_instance);
    s_instance = this;

#ifndef QT_NO_OPENGL
    //QGLWidget *wgl = new QGLWidget();
    //setupViewport(wgl);
#endif
}

PatienceView::~PatienceView()
{
    dscene()->clearHints();

    if (s_instance == this)
        s_instance = 0;

    delete dscene();
}

KXmlGuiWindow *PatienceView::mainWindow() const
{
    return m_window;
}


void PatienceView::setScene( QGraphicsScene *_scene )
{
    DealerScene *oldscene = dscene();
    QGraphicsView::setScene( _scene );
    resetCachedContent();
    delete oldscene;
    dscene()->rescale(true);

    if ( oldscene )
        dscene()->relayoutPiles();
}

PatienceView *PatienceView::instance()
{
    return s_instance;
}

DealerScene *PatienceView::dscene() const
{
    return dynamic_cast<DealerScene*>( scene() );
}

void PatienceView::wheelEvent( QWheelEvent *e )
{
    if ( e->modifiers() & Qt::ControlModifier )
    {
        qreal scaleFactor = pow((double)2, -e->delta() / (10*120.0));
        cardMap::self()->setWantedCardWidth( cardMap::self()->wantedCardWidth() / scaleFactor );
    }
}


void PatienceView::resizeEvent( QResizeEvent *e )
{
    if ( e )
        QGraphicsView::resizeEvent(e);

#if 0
    foreach (QWidget *widget, QApplication::allWidgets())
        kDebug(11111) << widget << " " << widget->objectName() << " " << widget->geometry();

    kDebug(11111) << "resizeEvent" << size() << " " << e << " " << dscene() << " " << parent()->isVisible(); /*<< " " << kBacktrace()*/
#endif

    if ( dscene() )
    {
        kDebug(11111) << "resizeEvent got through" << e->size();
        resetCachedContent();
        dscene()->setSceneSize( size() );
    }
    else
    {
        kDebug(11111) << "resizeEvent ignored" << e->size();
    }
}

#include "view.moc"
