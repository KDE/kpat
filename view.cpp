/*
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
*/
#include "view.h"
#include "dealer.h"
#include <kstaticdeleter.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include "deck.h"
#include <assert.h>
#include "kmainwindow.h"
#include <kapplication.h>
#include <kpixmapeffect.h>
#include <kxmlguifactory.h>
#include <kicon.h>
#include <QTimer>
//Added by qt3to4:
#include <QWheelEvent>
#include <QGraphicsSceneMouseEvent>
#include <QSvgRenderer>
#ifndef QT_NO_OPENGL
#include <QGLWidget>
#endif
#include <QPixmap>
#include <QDebug>
#include <QList>
#include <QTimeLine>
#include <QGraphicsItemAnimation>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QDomElement>
#include <krandom.h>
#include <kaction.h>
#include <klocale.h>
#include "cardmaps.h"
#include "speeds.h"
#include <kconfig.h>
#include <kglobal.h>
#include "version.h"
#include <ktoggleaction.h>

#include <math.h>

DealerInfoList *DealerInfoList::_self = 0;
static KStaticDeleter<DealerInfoList> dl;

DealerInfoList *DealerInfoList::self()
{
    if (!_self)
        _self = dl.setObject(_self, new DealerInfoList());
    return _self;
}

void DealerInfoList::add(DealerInfo *dealer)
{
    list.append(dealer);
}

// ================================================================
//                            class Dealer


PatienceView *PatienceView::s_instance = 0;

PatienceView::PatienceView( KMainWindow* _parent )
  : QGraphicsView( _parent ),
    ademo(0),
    ahint(0),
    aredeal(0),
    m_shown( false )
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setAlignment( Qt::AlignLeft | Qt::AlignTop );

    assert(!s_instance);
    s_instance = this;

    setCacheMode(QGraphicsView::CacheBackground);

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

KMainWindow *PatienceView::parent() const
{
    return dynamic_cast<KMainWindow*>(QGraphicsView::parent());
}


void PatienceView::setScene( QGraphicsScene *_scene )
{
    parent()->guiFactory()->unplugActionList( parent(), QString::fromLatin1("game_actions"));
    QGraphicsScene *oldscene = scene();
    QGraphicsView::setScene( _scene );
    delete oldscene;
    dscene()->rescale(true);
    dscene()->setGameNumber( KRandom::random() );
//    dscene()->setGameNumber( 1438470683 );

    // dscene()->setSceneRect( QRectF( 0,0,700,500 ) );
    scaleFactor = 1;
    dscene()->setItemIndexMethod(QGraphicsScene::NoIndex);
    // connect( _scene, SIGNAL( gameWon( bool ) ), SIGNAL( gameWon( bool ) ) );
    connect( dscene(), SIGNAL( undoPossible( bool ) ), SIGNAL( undoPossible( bool ) ) );

    if ( oldscene )
        dscene()->relayoutPiles();

    setupActions();
    if ( ademo ) {
        connect( dscene(), SIGNAL( demoActive( bool ) ), ademo, SLOT( setChecked( bool ) ) );
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
    QList<KAction*> actionlist;

    kDebug(11111) << "setupActions " << actions() << endl;

    if (dscene()->actions() & DealerScene::Hint) {

        ahint = new KAction( i18n("&Hint"),
                             parent()->actionCollection(), "game_hint");
        ahint->setCustomShortcut( Qt::Key_H );
        ahint->setIcon( KIcon( "wizard" ) );
        connect( ahint, SIGNAL( triggered( bool ) ), SLOT(hint()) );
        actionlist.append(ahint);
    } else
        ahint = 0;

    if (dscene()->actions() & DealerScene::Demo) {
        ademo = new KToggleAction( i18n("&Demo"), parent()->actionCollection(), "game_demo");
        ademo->setIcon( KIcon( "1rightarrow") );
        ademo->setCustomShortcut( Qt::CTRL+Qt::Key_D );
        connect( ademo, SIGNAL( triggered( bool ) ), dscene(), SLOT( toggleDemo(bool) ) );
        actionlist.append(ademo);
    } else
        ademo = 0;

    if (dscene()->actions() & DealerScene::Redeal) {
        aredeal = new KAction (i18n("&Redeal"), parent()->actionCollection(), "game_redeal");
        aredeal->setIcon( KIcon( "queue") );
        connect( aredeal, SIGNAL( triggered( bool ) ), dscene(), SLOT( redeal() ) );
        actionlist.append(aredeal);
    } else
        aredeal = 0;

    parent()->guiFactory()->plugActionList( parent(), QString::fromLatin1("game_actions"), actionlist);
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

    if ( !dscene() )
        return;

    dscene()->setSceneSize( size() );
}

#include "view.moc"
