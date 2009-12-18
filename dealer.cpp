/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2009 Parker Coates <parker.coates@gmail.com>
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

#include "dealer.h"

#include "carddeck.h"
#include "render.h"
#include "speeds.h"
#include "version.h"
#include "view.h"
#include "patsolve/patsolve.h"

#include <KConfigGroup>
#include <KDebug>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>
#include <KTemporaryFile>
#include <KIO/NetAccess>

#include <QtCore/QCoreApplication>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QGraphicsView>
#include <QtGui/QPainter>
#include <QtGui/QStyleOptionGraphicsItem>
#include <QtXml/QDomDocument>

#include <cassert>
#include <cmath>

#define DEBUG_HINTS 0

#if 0
inline void myassert_fail (__const char *__assertion, __const char *__file,
                           unsigned int __line, __const char *__function)
{
   QString tmp = PatienceView::instance()->dscene()->save_it();
   fprintf(stderr, "SAVE %s.saved\n", qPrintable(tmp));
   KIO::NetAccess::upload(tmp, tmp + ".saved", PatienceView::instance());
   KIO::NetAccess::del(tmp, PatienceView::instance());
   __assert_fail(__assertion, __file, __line, __function);
}

# define myassert(expr)                                                \
  ((expr)                                                               \
   ? __ASSERT_VOID_CAST (0)                                             \
   : myassert_fail (__STRING(expr), __FILE__, __LINE__, __ASSERT_FUNCTION) )
#endif

#define myassert assert


// ================================================================
//                         class MoveHint

MoveHint::MoveHint(Card *card, Pile *to, int prio)
{
    m_card         = card;
    m_to           = to;
    m_prio         = prio;
}


// ================================================================
//                          class CardState

class CardState {
public:
    Card *it;
    Pile *source;
    qreal z;
    bool faceup;
    bool tookdown;
    int source_index;
    CardState() {}
public:
    // as every card is only once we can sort after the card.
    // < is the same as <= in that context. == is different
    bool operator<(const CardState &rhs) const { return it < rhs.it; }
    bool operator<=(const CardState &rhs) const { return it <= rhs.it; }
    bool operator>(const CardState &rhs) const { return it > rhs.it; }
    bool operator>=(const CardState &rhs) const { return it > rhs.it; }
    bool operator==(const CardState &rhs) const {
        return (it == rhs.it && source == rhs.source &&
                z == rhs.z && faceup == rhs.faceup &&
                source_index == rhs.source_index && tookdown == rhs.tookdown);
    }
    void fillNode(QDomElement &e) const {
        e.setAttribute("value", it->rank());
        e.setAttribute("suit", it->suit());
        e.setAttribute("z", z);
        e.setAttribute("faceup", faceup);
        e.setAttribute("tookdown", tookdown);
    }
};

typedef class QList<CardState> CardStateList;

bool operator==( const State & st1, const State & st2) {
    return st1.cards == st2.cards && st1.gameData == st2.gameData;
}

class SolverThread : public QThread
{
public:
    virtual void run();
    SolverThread( Solver *solver) {
        m_solver = solver;
        ret = Solver::FAIL;
    }

    void finish()
    {
        {
            QMutexLocker lock( &m_solver->endMutex );
            m_solver->m_shouldEnd = true;
        }
        wait();
        ret = Solver::QUIT; // perhaps he managed to finish, but we won't listen
    }
    Solver::statuscode result() const { return ret; }

private:
    Solver *m_solver;
    Solver::statuscode ret;
};

void SolverThread::run()
{
    ret = Solver::FAIL;
    ret = m_solver->patsolve();
#if 0
    if ( ret == Solver::WIN )
    {
        kDebug() << "won\n";
    } else if ( ret == Solver::NOSOL )
    {
        kDebug() << "lost\n";
    } else if ( ret == Solver::QUIT  || ret == Solver::MLIMIT )
    {
        kDebug() << "quit\n";
    } else
        kDebug() << "unknown\n";
#endif
}

class DealerScene::DealerScenePrivate
{
public:
    bool _won;
    quint32 _id;
    bool _gameRecorded;
    bool gameStarted;
    bool wasJustSaved;
    QGraphicsPixmapItem *wonItem;
    bool gothelp;
    bool toldAboutLostGame;
    bool gameWasEverWinnable;
    // we need a flag to avoid telling someone the game is lost
    // just because the winning animation moved the cards away
    bool toldAboutWonGame;
    int myActions;
    Solver *m_solver;
    SolverThread *m_solver_thread;
    mutable QMutex solverMutex;
    QList<MOVE> winMoves;
    int neededFutureMoves;
    QTimer *updateSolver;
    int loadedMoveCount;
    Card *peekedCard;
    QTimer *demotimer;
    QTimer *stateTimer;
    QTimer *dropTimer;
    bool demo_active;
    bool stop_demo_next;
    qreal dropSpeedFactor;

    bool manualDropInProgress;
    QList<MoveHint*> hints;
    QList<State*> redoList;
    QList<State*> undoList;
};

int DealerScene::getMoves() const
{
    return d->loadedMoveCount + d->undoList.count();
}

void DealerScene::takeState()
{
    if ( !demoActive() )
    {
        d->winMoves.clear();
        clearHints();
    }
    if ( deck()->hasAnimatedCards() )
    {
        d->stateTimer->start();
        return;
    }

    d->stateTimer->stop();

    State *n = getState();

    if (!d->undoList.count()) {
        emit updateMoves(getMoves());
        d->undoList.append(n);
    } else {
        State *old = d->undoList.last();

        if (*old == *n) {
            delete n;
            n = 0;
        } else {
            emit updateMoves(getMoves());
            d->undoList.append(n);
        }
    }

    if (n) {
        d->wasJustSaved = false;
        if (isGameWon()) {
            won();
            return;
        }
        else if ( !d->toldAboutWonGame && !d->toldAboutLostGame && isGameLost() ) {
            emit hintPossible( false );
            emit demoPossible( false );
            emit solverStateChanged( i18n("Solver: This game is lost.") );
            d->toldAboutLostGame = true;
            stopDemo();
            return;
        }

        if ( d->m_solver && !demoActive() && !deck()->hasAnimatedCards() )
            startSolver();

        if (!demoActive() && !deck()->hasAnimatedCards())
            d->dropTimer->start( speedUpTime( TIME_BETWEEN_MOVES ) );

        emit undoPossible(d->undoList.count() > 1);
        emit redoPossible(d->redoList.count() > 1);
    }
}

void DealerScene::saveGame(QDomDocument &doc)
{
    QDomElement dealer = doc.createElement("dealer");
    doc.appendChild(dealer);
    dealer.setAttribute("id", gameId());
    dealer.setAttribute("options", getGameOptions());

    // If the game has been won, there's no sense in saving the state
    // across sessions. In that case, just save the game ID so that
    // we can later start up a new game of the same type.
    if ( isGameWon() )
        return;

    dealer.setAttribute("number", QString::number(gameNumber()));
    dealer.setAttribute("moves", getMoves() - 1);
    dealer.setAttribute("started", d->gameStarted);
    QString data = getGameState();
    if (!data.isEmpty())
        dealer.setAttribute("data", data);

    bool taken[1000];
    memset(taken, 0, sizeof(taken));

    foreach(const QGraphicsItem *item, items())
    {
        const Pile *p = qgraphicsitem_cast<const Pile*>(item);
        if (p)
        {
            if (taken[p->index()]) {
                kDebug() << "pile index" << p->index() << "taken twice\n";
                return;
            }
            taken[p->index()] = true;

            QDomElement pile = doc.createElement("pile");
            pile.setAttribute("index", p->index());
            pile.setAttribute("z", p->zValue());

            foreach(const Card * c, p->cards() )
            {
                QDomElement card = doc.createElement("card");
                card.setAttribute("suit", c->suit());
                card.setAttribute("value", c->rank());
                card.setAttribute("faceup", c->isFaceUp());
                card.setAttribute("z", c->realZ());
                pile.appendChild(card);
            }
            dealer.appendChild(pile);
        }
    }

    d->wasJustSaved = true;
}

void DealerScene::openGame(QDomDocument &doc)
{
    resetInternals();

    QDomElement dealer = doc.documentElement();

    QString options = dealer.attribute("options");

    // Before KDE4.3, KPat didn't store game specific options in the save
    // file. This could cause crashes when loading a Spider game with a
    // different number of suits than the current setting. Similarly, in 
    // Klondike the number of cards drawn from the deck was forgotten, but
    // this never caused crashes. Fortunately, in Spider we can count the
    // number of suits ourselves. For Klondike, there is no way to recover
    // that information.
    if (gameId() == 17 && options.isEmpty())
    {
        QSet<int> suits;
        QDomNodeList cardElements = dealer.elementsByTagName("card");
        for (int i = 0; i < cardElements.count(); ++i)
            suits.insert(cardElements.item(i).toElement().attribute("suit").toInt());
        options = QString::number(suits.count());
    }

    setGameOptions(options);
    setGameNumber(dealer.attribute("number").toInt());
    d->loadedMoveCount = dealer.attribute("moves").toInt();
    d->gameStarted = bool(dealer.attribute("started").toInt());

    QDomNodeList pileNodes = dealer.elementsByTagName("pile");

    deck()->returnAllCards();

    foreach (Pile *p, piles())
    {
        p->clear();
        for (int i = 0; i < pileNodes.count(); ++i)
        {
            QDomElement pile = pileNodes.item(i).toElement();
            if (pile.attribute("index").toInt() == p->index())
            {
                QDomNodeList cardNodes = pile.elementsByTagName("card");
                for (int j = 0; j < cardNodes.count(); ++j)
                {
                    QDomElement card = cardNodes.item(j).toElement();
                    Card::Suit s = static_cast<Card::Suit>(card.attribute("suit").toInt());
                    Card::Rank v = static_cast<Card::Rank>(card.attribute("value").toInt());

                    Card * c = deck()->takeCard( v, s );
                    Q_ASSERT( c );

                    c->turn(card.attribute("faceup").toInt());
                    p->add(c);
                }
                break;
            }
        }
        p->layoutCards( 0 );
    }
    setGameState( dealer.attribute("data") );

    emit updateMoves(getMoves());

    takeState();
}

void DealerScene::undo()
{
    clearHighlightedItems();
    stopDemo();
    kDebug() << "::undo" << d->undoList.count();
    if (d->undoList.count() > 1) {
        d->redoList.append( d->undoList.takeLast() );
        setState(d->undoList.takeLast());
        emit updateMoves(getMoves());
        takeState(); // copying it again
        emit undoPossible(d->undoList.count() > 1);
        emit redoPossible(d->redoList.count() > 0);
        if ( d->toldAboutLostGame ) { // everything's possible again
            hintPossible( true );
            demoPossible( true );
            d->toldAboutLostGame = false;
            d->toldAboutWonGame = false;
        }
    }
    emit solverStateChanged( QString() );
}

void DealerScene::redo()
{
    clearHighlightedItems();
    stopDemo();
    kDebug() << "::redo" << d->redoList.count();
    if (d->redoList.count() > 0) {
        setState(d->redoList.takeLast());
        emit updateMoves(getMoves());
        takeState(); // copying it again
        emit undoPossible(d->undoList.count() > 1);
        emit redoPossible(d->redoList.count() > 0);
        if ( d->toldAboutLostGame ) { // everything's possible again
            hintPossible( true );
            demoPossible( true );
            d->toldAboutLostGame = false;
            d->toldAboutWonGame = false;
        }
    }
    emit solverStateChanged( QString() );
}

void DealerScene::eraseRedo()
{
    qDeleteAll(d->redoList);
    d->redoList.clear();
    emit redoPossible( false );
}

// ================================================================
//                         class DealerScene


DealerScene::DealerScene()
  : _autodrop(true),
    gamenumber(0)
{
    setItemIndexMethod(QGraphicsScene::NoIndex);

    d = new DealerScenePrivate();
    d->demo_active = false;
    d->stop_demo_next = false;
    d->_won = false;
    d->_gameRecorded = false;
    d->gameStarted = false;
    d->wasJustSaved = false;
    d->gothelp = false;
    d->myActions = 0;
    d->m_solver = 0;
    d->m_solver_thread = 0;
    d->neededFutureMoves = 1;
    d->toldAboutLostGame = false;
    d->toldAboutWonGame = false;
    d->updateSolver = new QTimer(this);
    d->updateSolver->setInterval( 250 );
    d->updateSolver->setSingleShot( true );
    d->loadedMoveCount = 0;
    d->peekedCard = 0;
    connect( d->updateSolver, SIGNAL(timeout()), SLOT(stopAndRestartSolver()) );

    d->demotimer = new QTimer(this);
    connect( d->demotimer, SIGNAL(timeout()), SLOT(demo()) );
    d->dropSpeedFactor = 1;
    d->manualDropInProgress = false;

    d->stateTimer = new QTimer( this );
    connect( d->stateTimer, SIGNAL(timeout()), this, SLOT(takeState()) );
    d->stateTimer->setInterval( 100 );
    d->stateTimer->setSingleShot( true );

    d->dropTimer = new QTimer( this );
    connect( d->dropTimer, SIGNAL(timeout()), this, SLOT(drop()) );
    d->dropTimer->setSingleShot( true );

    d->wonItem  = new QGraphicsPixmapItem( 0, this );
    d->wonItem->setZValue( 2000 );
    d->wonItem->hide();

    _usesolver = true;
}

DealerScene::~DealerScene()
{
    clearHighlightedItems();

    clearHints();

    disconnect();
    if ( d->m_solver_thread )
        d->m_solver_thread->finish();
    delete d->m_solver_thread;
    d->m_solver_thread = 0;
    delete d->m_solver;
    d->m_solver = 0;
    qDeleteAll( d->undoList );
    qDeleteAll( d->redoList );
    delete d->wonItem;

    delete d;
}


// ----------------------------------------------------------------


QList<MoveHint*> DealerScene::hints() const
{
    return d->hints;
}

void DealerScene::hint()
{
    if ( deck()->hasAnimatedCards() && d->stateTimer->isActive() ) {
        QTimer::singleShot( 150, this, SLOT( hint() ) );
        return;
    }

    d->dropTimer->stop();

    if ( !highlightedItems().isEmpty() )
    {
        clearHighlightedItems();
        return;
    }

    clearHints();

    if ( d->winMoves.count() )
    {
        MOVE m = d->winMoves.first();
#if DEBUG_HINTS
        if ( m.totype == O_Type )
            fprintf( stderr, "move from %d out (at %d) Prio: %d\n", m.from,
                     m.turn_index, m.pri );
        else
            fprintf( stderr, "move from %d to %d (%d) Prio: %d\n", m.from, m.to,
                     m.turn_index, m.pri );
#endif

        MoveHint *mh = solver()->translateMove( m );
        if ( mh ) {
            newHint( mh );
            kDebug() << "win move" << mh->pile()->objectName() << " " << mh->card()->rank() << " " << mh->card()->suit() << " " << mh->priority();
        }
        getHints();
    } else {
        getHints();
    }

    QList<QGraphicsItem*> toHighlight;
    foreach ( MoveHint * h, d->hints )
        toHighlight << h->card();
    setHighlightedItems( toHighlight );

    clearHints();
}

void DealerScene::getSolverHints()
{
    if ( !d->solverMutex.tryLock() )
    {
        d->m_solver_thread->finish();
        // already locked
    }

    solver()->translate_layout();
    solver()->patsolve( 1 );

    kDebug() << "getSolverHints";
    QList<MOVE> moves = solver()->firstMoves;
    d->solverMutex.unlock();

    QListIterator<MOVE> mit( moves );

    while ( mit.hasNext() )
    {
        MOVE m = mit.next();
#if DEBUG_HINTS
        if ( m.totype == O_Type )
            fprintf( stderr, "   move from %d out (at %d) Prio: %d\n", m.from,
                     m.turn_index, m.pri );
        else
            fprintf( stderr, "   move from %d to %d (%d) Prio: %d\n", m.from, m.to,
                     m.turn_index, m.pri );
#endif

        MoveHint *mh = solver()->translateMove( m );
        if ( mh )
            newHint( mh );
    }
}

void DealerScene::getHints()
{
    if ( solver() )
    {
        getSolverHints();
        return;
    }

    foreach (Pile *store, piles())
    {
        if (store->target() || store->isEmpty())
            continue;

        CardList cards = store->cards();
        while (cards.count() && !cards.first()->realFace()) cards.erase(cards.begin());

        CardList::Iterator iti = cards.begin();
        while (iti != cards.end())
        {
            if (store->legalRemove(*iti)) {
                foreach (Pile *dest, piles())
                {
                    if (dest == store)
                        continue;
                    if (store->indexOf(*iti) == 0 && dest->isEmpty() && !dest->target())
                        continue;
                    if (!dest->legalAdd(cards))
                        continue;

                    bool old_prefer = checkPrefering( dest->checkIndex(), dest, cards );
                    if (dest->target())
                        newHint(new MoveHint(*iti, dest, 127));
                    else {
                        store->hideCards(cards);
                        // if it could be here as well, then it's no use
                        if ((store->isEmpty() && !dest->isEmpty()) || !store->legalAdd(cards))
                            newHint(new MoveHint(*iti, dest, 0));
                        else {
                            if (old_prefer && !checkPrefering( store->checkIndex(),
                                                               store, cards ))
                            { // if checkPrefers says so, we add it nonetheless
                                newHint(new MoveHint(*iti, dest, 10));
                            }
                        }
                        store->unhideCards(cards);
                    }
                }
            }
            cards.erase(iti);
            iti = cards.begin();
        }
    }
}

bool DealerScene::checkPrefering( int /*checkIndex*/, const Pile *, const CardList& ) const
{
    return false;
}

void DealerScene::clearHints()
{
    qDeleteAll( d->hints );
    d->hints.clear();
}

void DealerScene::newHint(MoveHint *mh)
{
    d->hints.append(mh);
}

void DealerScene::startNew(int gameNumber)
{
    if (gameNumber != -1)
        setGameNumber(gameNumber);

    // Only record the statistics and reset gameStarted if  we're starting a
    // new game number or we're restarting a game we've already won.
    if (gameNumber != -1 || d->_won)
    {
        recordGameStatistics();
        d->_gameRecorded = false;
        d->gameStarted = false;
    }

    if ( deck()->hasAnimatedCards() )
    {
        QTimer::singleShot( 100, this, SLOT( startNew() ) );
        return;
    }

    resetInternals();
    d->loadedMoveCount = 0;
    d->wasJustSaved = false;

    emit updateMoves( 0 );


    restart();
    takeState();
    update();
}

void DealerScene::resetInternals()
{
    stopDemo();
    clearHighlightedItems();

    d->_won = false;
    d->wonItem->hide();

    qDeleteAll(d->undoList);
    d->undoList.clear();
    qDeleteAll(d->redoList);
    d->redoList.clear();

    d->gameWasEverWinnable = false;
    d->toldAboutLostGame = false;
    d->toldAboutWonGame = false;

    d->gothelp = false;

    d->manualDropInProgress = false;
    d->dropSpeedFactor = 1;

    foreach (Card * c, deck()->cards())
    {
        c->disconnect( this );
        c->stopAnimation();
    }

    emit solverStateChanged( QString() );
    emit hintPossible( true );
    emit demoPossible( true );
}

QPointF posAlongRect( qreal distOnRect, const QRectF & rect )
{
    if ( distOnRect < rect.width() )
        return rect.topLeft() + QPointF( distOnRect, 0 );
    distOnRect -= rect.width();
    if ( distOnRect < rect.height() )
        return rect.topRight() + QPointF( 0, distOnRect );
    distOnRect -= rect.height();
    if ( distOnRect < rect.width() )
        return rect.bottomRight() + QPointF( -distOnRect, 0 );
    distOnRect -= rect.width();
    return rect.bottomLeft() + QPointF( 0, -distOnRect );
}

void DealerScene::won()
{
    if (d->_won)
        return;

    d->_won = true;
    d->toldAboutWonGame = true;

    stopDemo();
    recordGameStatistics();

    emit demoPossible( false );
    emit hintPossible( false );
    emit newCardsPossible( false );
    emit undoPossible( false );

    qreal speed = sqrt( width() * width() + height() * height() ) / ( DURATION_WON );

    QRectF justOffScreen = sceneRect().adjusted( -deck()->cardWidth(), -deck()->cardHeight(), 0, 0 );
    qreal spacing = 2 * ( justOffScreen.width() + justOffScreen.height() ) / deck()->cards().size();
    qreal distOnRect = 0;

    foreach ( Card *c, deck()->cards() )
    {
        distOnRect += spacing;
        QPointF pos2 = posAlongRect( distOnRect, justOffScreen );
        QPointF delta = c->pos() - pos2;
        qreal dist = sqrt( delta.x() * delta.x() + delta.y() * delta.y() );

        c->turn( true );
        c->animate( pos2, c->zValue(), 1, 0, true, false, dist / speed );
    }

    connect(deck(), SIGNAL(cardAnimationDone()), this, SLOT(showWonMessage()));
}

void DealerScene::showWonMessage()
{
    disconnect(deck(), SIGNAL(cardAnimationDone()), this, SLOT(showWonMessage()));

    // Return the Cards to the CardDeck, so they don't get shown if the Piles
    // try to layoutCards() if the window is resized.
    deck()->returnAllCards();

    updateWonItem();
    d->wonItem->show();
}

void DealerScene::updateWonItem()
{
    const qreal aspectRatio = Render::aspectRatioOfElement("message_frame");
    int boxWidth;
    int boxHeight;
    if (width() < aspectRatio * height())
    {
        boxWidth = width() * 0.7;
        boxHeight = boxWidth / aspectRatio;
    }
    else
    {
        boxHeight = height() * 0.7;
        boxWidth = boxHeight * aspectRatio;
    }

    // Only regenerate the pixmap if it doesn't already exist or the new one is a significantly different size.
    if ( qAbs( d->wonItem->boundingRect().width() - boxWidth ) > 20 )
    {
        QRect contentsRect = QRect( 0, 0, boxWidth, boxHeight );
        QPixmap pix = Render::renderElement( "message_frame", contentsRect.size() );

        QString text;
        if ( d->gothelp )
            text = i18n( "Congratulations! We have won." );
        else
            text = i18n( "Congratulations! You have won." );

        QFont font;
        int fontsize = 36;
        font.setPointSize( fontsize );
        int twidth = QFontMetrics( font ).width( text );
        while ( twidth > boxWidth * 0.9 && fontsize > 5 )
        {
            fontsize--;
            font.setPointSize( fontsize );
            twidth = QFontMetrics( font ).width ( text );
        }

        QPainter p( &pix );
        p.setFont( font );
        p.setPen( Render::colorOfElement("message_text_color") );
        p.drawText( contentsRect, Qt::AlignCenter, text );
        p.end();

        d->wonItem->setPixmap( pix );
    }

    d->wonItem->setPos( QPointF( ( width() - d->wonItem->sceneBoundingRect().width() ) / 2,
                                 ( height() - d->wonItem->sceneBoundingRect().height() ) / 2 ) + sceneRect().topLeft() );
}


void DealerScene::mousePressEvent( QGraphicsSceneMouseEvent * e )
{
    stopDemo();

    if ( e->button() == Qt::LeftButton )
    {
        if ( d->peekedCard )
            return;
        clearHighlightedItems();
        CardScene::mousePressEvent( e );
    }
    else if ( e->button() == Qt::RightButton && cardsBeingDragged().isEmpty() )
    {
        if ( deck()->hasAnimatedCards() )
            return;

        Card *card = qgraphicsitem_cast<Card*>( itemAt( e->scenePos() ) );
        if ( !card )
            return;

        if ( card != card->source()->top() )
        {
            d->peekedCard = card;
            QPointF pos2( card->x() + deck()->cardWidth() / 3.0, card->y() - deck()->cardHeight() / 4.0 );
            card->setZValue( card->zValue() + 0.1 );
            card->animate( pos2, card->zValue(), 1.1, 20, card->isFaceUp(), false, DURATION_FANCYSHOW );
        }
    }
}

void DealerScene::mouseReleaseEvent( QGraphicsSceneMouseEvent *e )
{
    clearHighlightedItems();

    if ( e->button() == Qt::RightButton )
    {
        if ( d->peekedCard && d->peekedCard->source() )
        {
            d->peekedCard->source()->layoutCards( DURATION_FANCYSHOW );
            d->peekedCard = 0;
            return;
        }

        Card * card = qgraphicsitem_cast<Card*>( itemAt( e->scenePos() ) );
        if ( card )
        {
            cardDoubleClicked( card ); // see bug #151921
            return;
        }
    }

    CardScene::mouseReleaseEvent( e );
}

void DealerScene::mouseDoubleClickEvent( QGraphicsSceneMouseEvent *e )
{
    if (d->demo_active) {
        stopDemo();
        return;
    }

    clearHighlightedItems();

    CardScene::mouseDoubleClickEvent( e );
}

bool DealerScene::cardDoubleClicked( Card * c )
{
    if (c->source()->cardDoubleClicked(c))
    {
        onGameStateAlteredByUser();
        return true;
    }

    if (c->animated())
        return false;

    if (c == c->source()->top()  && c->realFace() && c->source()->legalRemove( c )) {
        Pile *tgt = findTarget(c);
        if (tgt) {
            c->source()->moveCards(CardList() << c , tgt);
            onGameStateAlteredByUser();
            return true;
        }
    }
    return false;
}

State *DealerScene::getState()
{
    State * st = new State;
    foreach (Card *c, deck()->cards())
    {
       CardState s;
       s.it = c;
       s.source = c->source();
       if (!s.source) {
           kDebug() << c->objectName() << "has no parent\n";
           Q_ASSERT(false);
           continue;
       }
       s.source_index = c->source()->indexOf(c);
       s.z = c->realZ();
       s.faceup = c->realFace();
       s.tookdown = c->takenDown();
       st->cards.append(s);
    }
    qSort(st->cards);

    // Game specific information
    st->gameData = getGameState( );

    return st;
}

void DealerScene::setState(State *st)
{
    kDebug() << gettime() << "setState\n";
    CardStateList * n = &st->cards;

    foreach (Pile *p, piles())
    {
        foreach (Card *c, p->cards())
            c->setTakenDown(p->target());
        p->clear();
    }

    foreach (const CardState & s, *n)
    {
        Card *c = s.it;
        bool target = c->takenDown(); // abused
        c->turn(s.faceup);
        s.source->add(c, s.source_index);
        c->setZValue(s.z);
        c->setTakenDown(s.tookdown || (target && !s.source->target()));
    }

    foreach (Pile *p, piles())
        p->layoutCards(0);

    // restore game-specific information
    setGameState( st->gameData );

    delete st;
}

Pile *DealerScene::findTarget(Card *c)
{
    if (!c)
        return 0;

    foreach (Pile *p, piles())
    {
        if (!p->target())
            continue;
        if (p->legalAdd(CardList() << c))
            return p;
    }
    return 0;
}

void DealerScene::onGameStateAlteredByUser()
{
    d->gameStarted = true;
    takeState();
    eraseRedo();
}

void DealerScene::setAutoDropEnabled(bool a)
{
    _autodrop = a;
    d->dropTimer->start( TIME_BETWEEN_MOVES );
}

void DealerScene::startManualDrop()
{
    d->manualDropInProgress = true;
    d->dropTimer->start( TIME_BETWEEN_MOVES );
}

void DealerScene::setSolverEnabled(bool a)
{
    _usesolver = a;
}

bool DealerScene::drop()
{
    if (!autoDropEnabled() && !d->manualDropInProgress)
        return false;

    if (!cardsBeingDragged().isEmpty() || deck()->hasAnimatedCards() || d->undoList.isEmpty() ) {
        d->dropTimer->start( speedUpTime( TIME_BETWEEN_MOVES ) );
        return true;
    }

    kDebug() << gettime() << "startDrop \n";

    clearHighlightedItems();
    clearHints();
    getHints();

    foreach ( const MoveHint *mh, d->hints )
    {
        if ( mh->pile() && mh->pile()->target() && mh->priority() > 120 && !mh->card()->takenDown() )
        {
            Card *t = mh->card();
            CardList cards = mh->card()->source()->cards();
            while ( !cards.isEmpty() && cards.first() != t )
                cards.removeFirst();

            QMap<Card*,QPointF> oldPositions;
            foreach ( Card *c, cards )
                oldPositions.insert( c, c->pos() );

            t->source()->moveCards( cards, mh->pile() );

            bool animationStarted = false;
            int count = 0;
            foreach ( Card *c, cards )
            {
                QPointF destPos = c->realPos();
                c->stopAnimation();
                c->setPos( oldPositions.value( c ) );

                int duration = speedUpTime( DURATION_AUTODROP + count++ * DURATION_AUTODROP / 10 );
                c->moveTo( destPos, c->zValue(), duration );

                animationStarted |= c->animated();
            }

            d->dropSpeedFactor *= AUTODROP_SPEEDUP_FACTOR;

            if ( animationStarted )
                connect( deck(), SIGNAL(cardAnimationDone()), this, SLOT(waitForAutoDrop()) );

            return true;
        }
    }
    clearHints();
    d->dropSpeedFactor = 1;
    d->manualDropInProgress = false;

    return false;
}

int DealerScene::speedUpTime( int delay ) const
{
    if ( delay < DURATION_AUTODROP_MINIMUM )
        return delay;
    else
        return qMax<int>( delay * d->dropSpeedFactor, DURATION_AUTODROP_MINIMUM );
}

void DealerScene::stopAndRestartSolver()
{
    if ( d->toldAboutLostGame || d->toldAboutWonGame ) // who cares?
        return;

    if ( d->m_solver_thread && d->m_solver_thread->isRunning() )
    {
        d->m_solver_thread->finish();
        d->solverMutex.unlock();
    }

    if ( deck()->hasAnimatedCards() )
    {
        startSolver();
        return;
    }

    slotSolverEnded();
}

void DealerScene::slotSolverEnded()
{
    if ( d->m_solver_thread && d->m_solver_thread->isRunning() )
        return;
    if ( !d->solverMutex.tryLock() )
        return;
    d->m_solver->translate_layout();
    kDebug() << gettime() << "start thread\n";
    d->winMoves.clear();
    emit solverStateChanged( i18n("Solver: Calculating...") );
    if ( !d->m_solver_thread ) {
        d->m_solver_thread = new SolverThread( d->m_solver );
        connect( d->m_solver_thread, SIGNAL(finished()), this, SLOT(slotSolverFinished()));
    }
    d->m_solver_thread->start(_usesolver ? QThread::IdlePriority : QThread::NormalPriority );
}

void DealerScene::slotSolverFinished()
{
    Solver::statuscode ret = d->m_solver_thread->result();
    if ( d->solverMutex.tryLock() )
    {
        // if we can lock, then this finish signal is abnormal
        d->solverMutex.unlock();
        startSolver();
        return;
    }

    kDebug() << gettime() << "stop thread" << ret;
    switch (  ret )
    {
    case Solver::WIN:
        d->winMoves = d->m_solver->winMoves;
        d->solverMutex.unlock();
        d->gameWasEverWinnable = true;
        emit solverStateChanged( i18n("Solver: This game is winnable.") );
        break;
    case Solver::NOSOL:
        d->solverMutex.unlock();
        if ( d->gameWasEverWinnable )
            emit solverStateChanged( i18n("Solver: This game is no longer winnable.") );
        else
            emit solverStateChanged( i18n("Solver: This game cannot be won.") );
        break;
    case Solver::FAIL:
        d->solverMutex.unlock();
        emit solverStateChanged( i18n("Solver: Unable to determine if this game is winnable.") );
        break;
    case Solver::QUIT:
        d->solverMutex.unlock();
        startSolver();
        break;
    case Solver::MLIMIT:
        d->solverMutex.unlock();
        break;
    }
}

void DealerScene::waitForAutoDrop()
{
    disconnect( deck(), SIGNAL(cardAnimationDone()), this, SLOT(waitForAutoDrop()) );

    takeState();
    eraseRedo();
}

int DealerScene::gameNumber() const
{
    return gamenumber;
}

void DealerScene::setGameNumber(int gmn)
{
    // Deal in the range of 1 to INT_MAX.
    gamenumber = qMax(1, gmn);
    qDeleteAll(d->undoList);
    d->undoList.clear();
    qDeleteAll(d->redoList);
    d->redoList.clear();
}

void DealerScene::stopDemo()
{
    if (deck()->hasAnimatedCards()) {
        d->stop_demo_next = true;
        return;
    } else d->stop_demo_next = false;

    foreach (QGraphicsItem *item, items())
    {
        Card *c = qgraphicsitem_cast<Card*>(item);
        if (c)
            c->stopAnimation();
    }

    d->demotimer->stop();
    d->demo_active = false;
    emit demoActive( false );
}

bool DealerScene::demoActive() const
{
    return d->demo_active;
}

void DealerScene::toggleDemo()
{
    if ( !demoActive() )
        demo();
    else
        stopDemo();
}

MoveHint *DealerScene::chooseHint()
{
    kDebug() << "chooseHint " << d->winMoves.count();
    if ( d->winMoves.count() )
    {
        MOVE m = d->winMoves.first();
#if DEBUG_HINTS
        if ( m.totype == O_Type )
            fprintf( stderr, "move from %d out (at %d) Prio: %d\n", m.from,
                     m.turn_index, m.pri );
        else
            fprintf( stderr, "move from %d to %d (%d) Prio: %d\n", m.from, m.to,
                     m.turn_index, m.pri );
#endif
        MoveHint *mh = solver()->translateMove( m );

        if ( mh )
        {
            d->winMoves.removeFirst();
            d->hints.append( mh );
            return mh;
        }
    }

    if (d->hints.isEmpty())
        return 0;

    qSort( d->hints.begin(), d->hints.end() );
    return d->hints[0];
}

void DealerScene::demo()
{
    if ( deck()->hasAnimatedCards() || d->stateTimer->isActive() )
        return;

    if (d->stop_demo_next) {
        stopDemo();
        return;
    }
    d->stop_demo_next = false;
    d->demo_active = true;
    d->gothelp = true;
    d->gameStarted = true;
    clearHighlightedItems();
    clearHints();
    getHints();
    d->demotimer->stop();

    MoveHint *mh = chooseHint();
    if (mh) {
        kDebug(mh->pile()->top()) << "Moving" << mh->card()->objectName()
                                  << "from the" << mh->card()->source()->objectName()
                                  << "pile to the" << mh->pile()->objectName()
                                  << "pile, putting it on top of" << mh->pile()->top()->objectName();
        kDebug(!mh->pile()->top()) << "Moving" << mh->card()->objectName()
                                   << "from the" << mh->card()->source()->objectName()
                                   << "pile to the" << mh->pile()->objectName()
                                   << "pile, which is empty";
        myassert(mh->card()->source() == 0 ||
               mh->card()->source()->legalRemove(mh->card(), true));

        CardList empty;
        CardList cards = mh->card()->source()->cards();
        bool after = false;
        for (CardList::Iterator it = cards.begin(); it != cards.end(); ++it) {
            if (*it == mh->card())
                after = true;
            if (after)
                empty.append(*it);
        }

        myassert(!empty.isEmpty());

        QMap<Card*,QPointF> oldPositions;

        foreach (Card *c, empty) {
            c->stopAnimation();
            c->turn(true);
            oldPositions.insert(c, c->realPos());
        }

        assert(mh->card());
        assert(mh->card()->source());
        assert(mh->pile());
        assert(mh->card()->source() != mh->pile());
        assert(mh->pile()->target() || mh->pile()->legalAdd(empty));

        mh->card()->source()->moveCards(empty, mh->pile());

        foreach (Card *c, empty) {
            QPointF destPos = c->realPos();
            c->stopAnimation();
            c->setPos(oldPositions.value(c));
            c->moveTo(destPos, c->zValue(), DURATION_DEMO);
        }

        newDemoMove(mh->card());

    } else {
        kDebug() << "demoNewCards";
        Card *t = newCards();
        if (t) {
            newDemoMove(t);
        } else if (isGameWon()) {
            won();
            return;
        } else {
            stopDemo();
            slotSolverEnded();
            return;
        }
    }

    emit demoActive( true );
    takeState();
    eraseRedo();
}

Card *DealerScene::newCards()
{
    return 0;
}

void DealerScene::newDemoMove(Card *m)
{
    kDebug() << "newDemoMove" << m->rank() << " " << m->suit();
    if ( m->animated() )
        connect(m, SIGNAL(animationStopped(Card*)), SLOT(waitForDemo(Card*)));
    else
        waitForDemo( 0 );
}

void DealerScene::waitForDemo(Card *t)
{
    if ( t )
    {
        kDebug() << "waitForDemo" << t->rank() << " " << t->suit();
        t->disconnect(this, SLOT(waitForDemo(Card*)) );
        takeState();
    }
    d->demotimer->start(250);
}

void DealerScene::setSolver( Solver *s) {
    delete d->m_solver;
    delete d->m_solver_thread;
    d->m_solver = s;
    d->m_solver_thread = 0;
}

bool DealerScene::isGameWon() const
{
    foreach (Pile *p, piles())
    {
        if (!p->target() && !p->isEmpty())
            return false;
    }
    return true;
}

void DealerScene::startSolver() const
{
    if(_usesolver)
        d->updateSolver->start();
}

void DealerScene::unlockSolver() const
{
    d->solverMutex.unlock();
}

void DealerScene::finishSolver() const
{
    if ( !d->solverMutex.tryLock() )
    {
        d->m_solver_thread->finish();
        unlockSolver();
    } else
        d->solverMutex.unlock(); // have to unlock again
}

bool DealerScene::isGameLost() const
{
    if ( solver() )
    {
        finishSolver();
        QMutexLocker ml( &d->solverMutex );
        solver()->translate_layout();
        Solver::statuscode ret = solver()->patsolve( neededFutureMoves() );
        if ( ret == Solver::NOSOL )
            return true;
        return false;
    }
    return false;
}

void DealerScene::recordGameStatistics()
{
    // Don't record the game if it was never started, if it is unchanged since
    // it was last saved (allowing the user to close KPat after saving without
    // it recording a loss) or if it has already been recorded.
    if ( d->gameStarted && !d->wasJustSaved && !d->_gameRecorded )
    {
        int id = oldId();

        QString totalPlayedKey = QString("total%1").arg( id );
        QString wonKey = QString("won%1").arg( id );
        QString winStreakKey = QString("winstreak%1").arg( id );
        QString maxWinStreakKey = QString("maxwinstreak%1").arg( id );
        QString loseStreakKey = QString("loosestreak%1").arg( id );
        QString maxLoseStreakKey = QString("maxloosestreak%1").arg( id );

        KConfigGroup config(KGlobal::config(), scores_group);

        int totalPlayed = config.readEntry( totalPlayedKey, 0 );
        int won = config.readEntry( wonKey, 0 );
        int winStreak = config.readEntry( winStreakKey, 0 );
        int maxWinStreak = config.readEntry( maxWinStreakKey, 0 );
        int loseStreak = config.readEntry( loseStreakKey, 0 );
        int maxLoseStreak = config.readEntry( maxLoseStreakKey, 0 );

        ++totalPlayed;

        if ( d->_won )
        {
            ++won;
            ++winStreak;
            maxWinStreak = qMax( winStreak, maxWinStreak );
            loseStreak = 0;
        }
        else
        {
            ++loseStreak;
            maxLoseStreak = qMax( loseStreak, maxLoseStreak );
            winStreak = 0;
        }

        config.writeEntry( totalPlayedKey, totalPlayed );
        config.writeEntry( wonKey, won );
        config.writeEntry( winStreakKey, winStreak );
        config.writeEntry( maxWinStreakKey, maxWinStreak );
        config.writeEntry( loseStreakKey, loseStreak );
        config.writeEntry( maxLoseStreakKey, maxLoseStreak );

        d->_gameRecorded = true;
    }
}

void DealerScene::relayoutScene()
{
    CardScene::relayoutScene();

    if ( d->wonItem->isVisible() )
        updateWonItem();
}

void DealerScene::setGameId(int id) { d->_id = id; }
int DealerScene::gameId() const { return d->_id; }

void DealerScene::setActions(int actions) { d->myActions = actions; }
int DealerScene::actions() const { return d->myActions; }

QList<QAction*> DealerScene::configActions() const { return QList<QAction*>(); }

Solver *DealerScene::solver() const { return d->m_solver; }

int DealerScene::neededFutureMoves() const { return d->neededFutureMoves; }
void DealerScene::setNeededFutureMoves( int i ) { d->neededFutureMoves = i; }


QString DealerScene::save_it()
{
    KTemporaryFile file;
    file.setAutoRemove(false);
    file.open();
    QDomDocument doc("kpat");
    saveGame(doc);
    QTextStream stream (&file);
    stream << doc.toString();
    stream.flush();
    file.flush();
    // .close will make fileName() return ""
    QString filename = file.fileName();
    file.close();
    return filename;
}

void DealerScene::createDump( QPaintDevice *device )
{
    int maxz = 0;
    foreach (QGraphicsItem *item, items())
    {
        assert( item->zValue() >= 0 );
        maxz = qMax( maxz, int( item->zValue() ) );
    }
    QStyleOptionGraphicsItem options;
    QPainter p( device );

    for ( int z = 0; z <= maxz; ++z )
    {
        foreach (QGraphicsItem *item, items())
        {
            if ( !item->isVisible() || item->zValue() != z )
                continue;

            Pile * pile = qgraphicsitem_cast<Pile*>(item);
            if ( pile )
            {
                p.save();
                p.setTransform(item->deviceTransform(p.worldTransform()), false);
                p.drawPixmap( 0, 0, pile->pixmap() );
                p.restore();
                continue;
            }
            else if ( !qgraphicsitem_cast<Card*>(item) )
            {
                kDebug() << "Unknown item type";
                assert( false );
            }

            p.save();
            p.setTransform(item->deviceTransform(p.worldTransform()), false);
            item->paint( &p, &options );
            p.restore();
        }
    }
}

void DealerScene::mapOldId(int) {}


int DealerScene::oldId() const
{
    return gameId();
}


bool DealerScene::allowedToStartNewGame()
{
    // Check if the user is already running a game, and if she is,
    // then ask if she wants to abort it.
    return !d->gameStarted
           || d->wasJustSaved
           || isGameWon()
           || isGameLost()
           || KMessageBox::warningContinueCancel(0,
                     i18n("A new game has been requested, but there is already a game in progress.\n\n"
                          "A loss will be recorded in the statistics if the current game is abandoned."),
                     i18n("Abandon Current Game?"),
                     KGuiItem(i18n("Abandon Current Game")),
                     KStandardGuiItem::cancel(),
                     "careaboutstats"
                    ) == KMessageBox::Continue;
}

void DealerScene::addCardForDeal(Pile * pile, Card * card, bool faceUp, QPointF startPos)
{
    Q_ASSERT( card );
    Q_ASSERT( pile );

    card->turn( faceUp );
    pile->add( card );
    m_initDealPositions.insert( card, startPos );
}


void DealerScene::startDealAnimation()
{
    qreal speed = sqrt( width() * width() + height() * height() ) / ( DURATION_DEAL );
    foreach ( Pile * p, piles() )
    {
        p->layoutCards(0);
        foreach ( Card * c, p->cards() )
        {
            if ( !m_initDealPositions.contains( c ) )
                continue;

            QPointF pos2 = c->pos();
            c->setPos( m_initDealPositions.value( c ) );

            QPointF delta = c->pos() - pos2;
            qreal dist = sqrt( delta.x() * delta.x() + delta.y() * delta.y() );
            int duration = qRound( dist / speed );
            c->animate( pos2, c->zValue(), 1, 0, c->isFaceUp(), false, duration );
        }
    }
    m_initDealPositions.clear();
}



#include "dealer.moc"
