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
#include "deck.h"
#include "cardmaps.h"
#include "version.h"

#include <cassert>
#include <cmath>

#include <QtCore/QCoreApplication>
#include <QtCore/QList>
#include <QtGui/QAction>
#include <QtGui/QResizeEvent>
#include <QtGui/QWheelEvent>

#ifndef QT_NO_OPENGL
#endif

#include <kaction.h>
#include <ktoggleaction.h>
#include <kstandardgameaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kicon.h>
#include <klocalizedstring.h>
#include <krandom.h>
#include <kxmlguifactory.h>
#include <kxmlguiwindow.h>

DealerInfoList *DealerInfoList::_self = 0;

void DealerInfoList::cleanupDealerInfoList()
{
    delete DealerInfoList::_self;
}

DealerInfoList *DealerInfoList::self()
{
    if (!_self) {
        _self = new DealerInfoList();
        qAddPostRoutine(DealerInfoList::cleanupDealerInfoList);
    }
    return _self;
}

void DealerInfoList::add(DealerInfo *dealer)
{
    list.append(dealer);
}

// ================================================================
//                            class Dealer


PatienceView *PatienceView::s_instance = 0;

PatienceView::PatienceView( KXmlGuiWindow* _window, QWidget* _parent )
  : QGraphicsView( _parent ),
    ademo(0),
    ahint(0),
    aredeal(0),
    adrop(0),
    m_shown( true ),
    m_window( _window )
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(QFrame::NoFrame);
    setAlignment( Qt::AlignLeft | Qt::AlignTop );
    setCacheMode(QGraphicsView::CacheBackground);

    assert(!s_instance);
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

    // dscene()->setSceneRect( QRectF( 0,0,700,500 ) );
    scaleFactor = 1;
    dscene()->setItemIndexMethod(QGraphicsScene::NoIndex);
    // connect( _scene, SIGNAL( gameWon( bool ) ), SIGNAL( gameWon( bool ) ) );

    if ( oldscene )
        dscene()->relayoutPiles();

    setupActions();
}

PatienceView *PatienceView::instance()
{
    return s_instance;
}

void PatienceView::setupActions()
{
    QList<QAction*> actionlist;

    mainWindow()->guiFactory()->unplugActionList( mainWindow(), QString::fromLatin1("game_actions"));

    kDebug(11111) << "setupActions" << actions();

    if (dscene()->actions() & DealerScene::Hint) {
        ahint = KStandardGameAction::hint(this, SLOT(hint()), mainWindow()->actionCollection());
        connect( dscene(), SIGNAL( hintPossible( bool ) ), ahint, SLOT( setEnabled( bool ) ) );
        actionlist.append(ahint);
    } else
        ahint = 0;

    if (dscene()->actions() & DealerScene::Demo) {
        ademo = KStandardGameAction::demo(dscene(), SLOT(toggleDemo()), mainWindow()->actionCollection());
        connect( dscene(), SIGNAL( demoActive( bool ) ), this, SLOT( toggleDemo( bool ) ) );
        connect( dscene(), SIGNAL( demoPossible( bool ) ), ademo, SLOT( setEnabled( bool ) ) );
        actionlist.append(ademo);
    } else
        ademo = 0;

    if (dscene()->actions() & DealerScene::Redeal) {
        aredeal = mainWindow()->actionCollection()->addAction( "game_redeal" );
        aredeal->setText( i18n("&Redeal") );
        aredeal->setIcon( KIcon( "roll") );
        connect( dscene(), SIGNAL( redealPossible( bool ) ), aredeal, SLOT( setEnabled( bool ) ) );
        connect( aredeal, SIGNAL( triggered( bool ) ), dscene(), SLOT( redeal() ) );
        actionlist.append(aredeal);
    } else
        aredeal = 0;

    delete adrop;
    if ( !dscene()->autoDrop() ) {
        adrop = mainWindow()->actionCollection()->addAction( "autodrop" );
        adrop->setText( i18n("Drop!") );
        adrop->setIcon( KIcon( "legalmoves") );
        connect( adrop, SIGNAL( triggered( bool ) ), dscene(), SLOT( slotAutoDrop() ) );
        actionlist.append(adrop);
    } else
        adrop = 0;

    mainWindow()->guiFactory()->plugActionList( mainWindow(), QString::fromLatin1("game_actions"), actionlist);
}


void PatienceView::toggleDemo( bool flag )
{
    kDebug(11111) << flag;
    ademo->setChecked( flag );
    if ( !flag )
        ademo->setIcon( KIcon( "media-playback-start") );
    else
        ademo->setIcon( KIcon( "media-playback-pause") );
}

void PatienceView::hint()
{
    dscene()->hint();
}

void PatienceView::setSolverEnabled(bool a)
{
    dscene()->setSolverEnabled( a );
}

void PatienceView::setAutoDropEnabled(bool a)
{
    dscene()->setAutoDropEnabled( a );
    setupActions();
}

void PatienceView::startNew(long gameNumber)
{
    kDebug(11111) << "startnew\n";
    if ( ahint )
        ahint->setEnabled( true );
    if ( ademo )
        ademo->setEnabled( true );
    if ( aredeal )
        aredeal->setEnabled( true );
    dscene()->startNew( gameNumber );
}

void PatienceView::slotEnableRedeal( bool en )
{
    if ( aredeal )
        aredeal->setEnabled( en );
}

DealerScene *PatienceView::dscene() const
{
    return dynamic_cast<DealerScene*>( scene() );
}

void PatienceView::setAnchorName(const QString &name)
{
    kDebug(11111) << "setAnchorname" << name;
    ac = name;
}

QString PatienceView::anchorName() const { return ac; }

void PatienceView::wheelEvent( QWheelEvent *e )
{
    return; // Maren hits the wheel mouse function of the touch pad way too often :)
    qreal scaleFactor = pow((double)2, -e->delta() / (10*120.0));
    cardMap::self()->setWantedCardWidth( cardMap::self()->wantedCardWidth() / scaleFactor );
}


void PatienceView::resizeEvent( QResizeEvent *e )
{
    if ( e )
        QGraphicsView::resizeEvent(e);

    kDebug(11111) << "resizeEvent" << wasShown();

    if ( !wasShown() )
        return;

#if 0
    foreach (QWidget *widget, QApplication::allWidgets())
        kDebug() << widget << " " << widget->objectName() << " " << widget->geometry();

    kDebug() << "resizeEvent" << size() << " " << e << " " << dscene() << " " << parent()->isVisible(); /*<< " " << kBacktrace()*/
#endif

    resetCachedContent();

    if ( !dscene() )
        return;

    dscene()->setSceneSize( size() );
}

#include "view.moc"
