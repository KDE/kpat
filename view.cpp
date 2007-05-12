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

#include <QtCore/QCoreApplication>
#include <QtCore/QList>
#include <QtGui/QAction>
#include <QtGui/QResizeEvent>
#include <QtGui/QWheelEvent>

#ifndef QT_NO_OPENGL
#include <QtOpenGL/QGLWidget>
#endif

#include <assert.h>
#include <math.h>

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

PatienceView::PatienceView( KXmlGuiWindow* _parent )
  : QGraphicsView( _parent ),
    ademo(0),
    ahint(0),
    aredeal(0),
    m_shown( false )
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(QFrame::NoFrame);
    setAlignment( Qt::AlignLeft | Qt::AlignTop );

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
}

KXmlGuiWindow *PatienceView::parent() const
{
    return dynamic_cast<KXmlGuiWindow*>(QGraphicsView::parent());
}


void PatienceView::setScene( QGraphicsScene *_scene )
{
    parent()->guiFactory()->unplugActionList( parent(), QString::fromLatin1("game_actions"));
    DealerScene *oldscene = dscene();
    QGraphicsView::setScene( _scene );
    resetCachedContent();
    delete oldscene;
    dscene()->rescale(true);
    dscene()->setGameNumber( KRandom::random() );

    // dscene()->setSceneRect( QRectF( 0,0,700,500 ) );
    scaleFactor = 1;
    dscene()->setItemIndexMethod(QGraphicsScene::NoIndex);
    // connect( _scene, SIGNAL( gameWon( bool ) ), SIGNAL( gameWon( bool ) ) );

    if ( oldscene )
        dscene()->relayoutPiles();

    setupActions();
    if ( ademo ) {
        connect( dscene(), SIGNAL( demoActive( bool ) ), this, SLOT( toggleDemo( bool ) ) );
        connect( dscene(), SIGNAL( demoPossible( bool ) ), ademo, SLOT( setEnabled( bool ) ) );
    }
    if ( ahint )
        connect( dscene(), SIGNAL( hintPossible( bool ) ), ahint, SLOT( setEnabled( bool ) ) );
    if ( aredeal )
        connect( dscene(), SIGNAL( redealPossible( bool ) ), aredeal, SLOT( setEnabled( bool ) ) );
}

PatienceView *PatienceView::instance()
{
    return s_instance;
}

void PatienceView::setupActions()
{
    QList<QAction*> actionlist;

    kDebug(11111) << "setupActions " << actions() << endl;

    if (dscene()->actions() & DealerScene::Hint) {
        ahint = KStandardGameAction::hint(this, SLOT(hint()), parent()->actionCollection());
        actionlist.append(ahint);
    } else
        ahint = 0;

    if (dscene()->actions() & DealerScene::Demo) {
        ademo = KStandardGameAction::demo(dscene(), SLOT(toggleDemo()), parent()->actionCollection());
        actionlist.append(ademo);
    } else
        ademo = 0;

    if (dscene()->actions() & DealerScene::Redeal) {
        aredeal = parent()->actionCollection()->addAction( "game_redeal" );
        aredeal->setText( i18n("&Redeal") );
        aredeal->setIcon( KIcon( "queue") );
        connect( aredeal, SIGNAL( triggered( bool ) ), dscene(), SLOT( redeal() ) );
        actionlist.append(aredeal);
    } else
        aredeal = 0;

    parent()->guiFactory()->plugActionList( parent(), QString::fromLatin1("game_actions"), actionlist);
}


void PatienceView::toggleDemo( bool flag )
{
    if ( !flag )
        ademo->setIcon( KIcon( "media-playback-start") );
    else
        ademo->setIcon( KIcon( "media-playback-pause") );
}

void PatienceView::hint()
{
    dscene()->hint();
}

void PatienceView::setAutoDropEnabled(bool a)
{
    dscene()->setAutoDropEnabled( a );
}

void PatienceView::startNew()
{
    kDebug() << "startnew\n";
    if ( ahint )
        ahint->setEnabled( true );
    if ( ademo )
        ademo->setEnabled( true );
    if ( aredeal )
        aredeal->setEnabled( true );
    dscene()->startNew();
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

void PatienceView::setGameNumber(long gmn) { dscene()->setGameNumber(gmn); }
long PatienceView::gameNumber() const { return dscene()->gameNumber(); }


void PatienceView::setAnchorName(const QString &name)
{
    kDebug(11111) << "setAnchorname " << name << endl;
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

    kDebug() << "resizeEvent " << wasShown() << endl;

    if ( !wasShown() )
        return;

#if 0
    foreach (QWidget *widget, QApplication::allWidgets())
        kDebug() << widget << " " << widget->objectName() << " " << widget->geometry() << endl;

    kDebug() << "resizeEvent " << size() << " " << e << " " << dscene() << " " << parent()->isVisible() << /* " " << kBacktrace() << */ endl;
#endif

    setCacheMode(QGraphicsView::CacheBackground);
    resetCachedContent();

    if ( !dscene() )
        return;

    dscene()->setSceneSize( size() );
}

#include "view.moc"
