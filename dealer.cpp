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

#include "dealer.h"
#include "deck.h"
#include "cardmaps.h"
#include "speeds.h"
#include "version.h"
#include "view.h"
#include "patsolve/patsolve.h"
#include "render.h"

#include <cassert>
#include <cmath>

#include <QStyleOptionGraphicsItem>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QGraphicsView>
#include <QtGui/QPainter>
#include <QtXml/QDomDocument>

#include <KTemporaryFile>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <klocalizedstring.h>
#include <krandom.h>
#include <ksharedconfig.h>
#include <kio/netaccess.h>

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

    void finish() {
        m_solver->m_shouldEnd = true;
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
    QGraphicsPixmapItem *wonItem;
    bool gothelp;
    bool toldAboutLostGame;
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
    bool hasScreenRect;

    QTimer *demotimer;
    QTimer *stateTimer;
    QTimer *autoDropTimer;
    bool demo_active;
    bool stop_demo_next;
    qreal m_autoDropFactor;

    bool initialDeal;
    qreal offx;
    qreal offy;
    bool m_autoDropOnce;
    QList<State*> redoList;
    QList<State*> undoList;
};

int DealerScene::getMoves() const
{
    return d->undoList.count();
}

void DealerScene::takeState()
{
    //kDebug(11111) << "takeState" << waiting();

    if ( !demoActive() )
    {
        //kDebug(11111) << "clear in takeState";
        d->winMoves.clear();
        clearHints();
    }
    if ( waiting() )
    {
        d->stateTimer->start();
        return;
    }

    QList<QGraphicsItem *> list = items();
    for (QList<QGraphicsItem *>::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it)
    {
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId ) {
            Card *c = static_cast<Card*>(*it);
            if (c->animated())
            {
                d->stateTimer->start();
                return;
            }
        }
    }

    d->stateTimer->stop();

    State *n = getState();

    if (!d->undoList.count()) {
        emit updateMoves();
        d->undoList.append(n);
    } else {
        State *old = d->undoList.last();

        if (*old == *n) {
            delete n;
            n = 0;
        } else {
            emit updateMoves();
            d->undoList.append(n);
        }
    }

    if (n) {
        d->initialDeal = false;
        if (isGameWon()) {
            won();
            d->toldAboutWonGame = true;
            return;
        }
        else if ( !d->toldAboutWonGame && !d->toldAboutLostGame && isGameLost() ) {
            emit hintPossible( false );
            emit demoPossible( false );
            emit redealPossible( false );
            QTimer::singleShot(400, this, SIGNAL(gameLost()));
            d->toldAboutLostGame = true;
            stopDemo();
            return;
        }

        if ( d->m_solver && !demoActive() && !waiting() )
            startSolver();

        if (!demoActive() && !waiting())
            d->autoDropTimer->start( speedUpTime( TIME_BETWEEN_MOVES ) );

        emit undoPossible(d->undoList.count() > 1);
        emit redoPossible(d->redoList.count() > 1);
    }
}

bool DealerScene::isInitialDeal() const { return d->initialDeal; }

void DealerScene::saveGame(QDomDocument &doc)
{
    QDomElement dealer = doc.createElement("dealer");
    doc.appendChild(dealer);
    dealer.setAttribute("id", gameId());
    dealer.setAttribute("number", QString::number(gameNumber()));
    QString data = getGameState();
    if (!data.isEmpty())
        dealer.setAttribute("data", data);
    dealer.setAttribute("moves", QString::number(getMoves()));

    bool taken[1000];
    memset(taken, 0, sizeof(taken));

    QList<QGraphicsItem *> list = items();
    for (QList<QGraphicsItem *>::Iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::PileTypeId ) {
            Pile *p = dynamic_cast<Pile*>(*it);
            assert(p);
            if (taken[p->index()]) {
                kDebug(11111) << "pile index" << p->index() << "taken twice\n";
                return;
            }
            taken[p->index()] = true;

            QDomElement pile = doc.createElement("pile");
            pile.setAttribute("index", p->index());
            pile.setAttribute("z", p->zValue());

            CardList cards = p->cards();
            for (CardList::Iterator it = cards.begin();
                 it != cards.end();
                 ++it)
            {
                QDomElement card = doc.createElement("card");
                card.setAttribute("suit", (*it)->suit());
                card.setAttribute("value", (*it)->rank());
                card.setAttribute("faceup", (*it)->isFaceUp());
                card.setAttribute("z", (*it)->realZ());
                pile.appendChild(card);
            }
            dealer.appendChild(pile);
        }
    }

    /*
    QDomElement eList = doc.createElement("undo");

    QPtrListIterator<State> it(undoList);
    for (; it.current(); ++it)
    {
        State *n = it.current();
        QDomElement state = doc.createElement("state");
        if (!n->gameData.isEmpty())
            state.setAttribute("data", n->gameData);
        QDomElement cards = doc.createElement("cards");
        for (QValueList<CardState>::ConstIterator it2 = n->cards.constBegin();
             it2 != n->cards.constEnd(); ++it2)
        {
            QDomElement item = doc.createElement("item");
            (*it2).fillNode(item);
            cards.appendChild(item);
        }
        state.appendChild(cards);
        eList.appendChild(state);
    }
    dealer.appendChild(eList);
    */
    // kDebug(11111) << doc.toString();
}

void DealerScene::openGame(QDomDocument &doc)
{
    unmarkAll();
    QDomElement dealer = doc.documentElement();

    setGameNumber(dealer.attribute("number").toULong());

    QDomNodeList piles = dealer.elementsByTagName("pile");

    QList<QGraphicsItem *> list = items();

    CardList cards;
    for (QList<QGraphicsItem *>::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it)
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId )
            cards.append(static_cast<Card*>(*it));

    Deck::deck()->collectAndShuffle();

    for (QList<QGraphicsItem *>::Iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::PileTypeId )
        {
            Pile *p = dynamic_cast<Pile*>(*it);
            assert(p);

            p->clear();

            for (int i = 0; i < piles.count(); ++i)
            {
                QDomElement pile = piles.item(i).toElement();
                if (pile.attribute("index").toInt() == p->index())
                {
                    QDomNodeList pcards = pile.elementsByTagName("card");
                    for (int j = 0; j < pcards.count(); ++j)
                    {
                        QDomElement card = pcards.item(j).toElement();
                        Card::Suit s = static_cast<Card::Suit>(card.attribute("suit").toInt());
                        Card::Rank v = static_cast<Card::Rank>(card.attribute("value").toInt());

                        for (CardList::Iterator it2 = cards.begin();
                             it2 != cards.end(); ++it2)
                        {

                            if ((*it2)->suit() == s && (*it2)->rank() == v)
                            {
                                (*it2)->setVisible(p->isVisible());
                                p->add(*it2, !card.attribute("faceup").toInt());
                                (*it2)->stopAnimation();
                                (*it2)->setVisible(p->isVisible());
                                cards.erase(it2);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    setGameState( dealer.attribute("data") );

    if (d->undoList.count() > 1) {
        setState(d->undoList.takeLast());
        takeState(); // copying it again
        eraseRedo();
        emit undoPossible(d->undoList.count() > 1);
    }

    emit updateMoves();
    takeState();
}

void DealerScene::undo()
{
    unmarkAll();
    stopDemo();
    kDebug(11111) << "::undo" << d->undoList.count();
    if (d->undoList.count() > 1) {
        d->redoList.append( d->undoList.takeLast() );
        setState(d->undoList.takeLast());
        emit updateMoves();
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
    emit gameSolverUnknown();
}

void DealerScene::redo()
{
    unmarkAll();
    stopDemo();
    kDebug(11111) << "::redo" << d->redoList.count();
    if (d->redoList.count() > 0) {
        setState(d->redoList.takeLast());
        emit updateMoves();
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
    emit gameSolverUnknown();
}

void DealerScene::eraseRedo()
{
    qDeleteAll(d->redoList);
    d->redoList.clear();
    emit redoPossible( false );
}

// ================================================================
//                         class DealerScene


DealerScene::DealerScene():
    _autodrop(true),
    _waiting(0),
    gamenumber( 0 )
{
    d = new DealerScenePrivate();
    d->demo_active = false;
    d->stop_demo_next = false;
    d->_won = false;
    d->_gameRecorded = false;
    d->wonItem = 0;
    d->gothelp = false;
    d->myActions = 0;
    d->m_solver = 0;
    d->m_solver_thread = 0;
    d->offx = d->offy = 0;
    d->neededFutureMoves = 1;
    d->toldAboutLostGame = false;
    d->toldAboutWonGame = false;
    d->hasScreenRect = false;
    d->updateSolver = new QTimer(this);
    d->updateSolver->setInterval( 250 );
    d->updateSolver->setSingleShot( true );
    d->initialDeal = true;
    connect( d->updateSolver, SIGNAL( timeout() ), SLOT( stopAndRestartSolver() ) );

    d->demotimer = new QTimer(this);
    connect(d->demotimer, SIGNAL(timeout()), SLOT(demo()));
    d->m_autoDropFactor = 1;
    d->m_autoDropOnce = false;
    connect( this, SIGNAL( gameWon( bool ) ), SLOT( showWonMessage() ) );

    d->stateTimer = new QTimer( this );
    connect( d->stateTimer, SIGNAL( timeout() ), this, SLOT( takeState() ) );
    d->stateTimer->setInterval( 100 );
    d->stateTimer->setSingleShot( true );

    d->autoDropTimer = new QTimer( this );
    connect( d->autoDropTimer, SIGNAL( timeout() ), this, SLOT( startAutoDrop() ) );
    d->autoDropTimer->setSingleShot( true );
}

DealerScene::~DealerScene()
{
    kDebug(11111) << "~DealerScene";
    if (!d->_won)
        countLoss();
    unmarkAll();

    // don't delete the deck
    if ( Deck::deck()->scene() == this )
    {
        Deck::deck()->setParent( 0 );
        removeItem( Deck::deck() );
    }
    removePile( Deck::deck() );
    while (!piles.isEmpty()) {
        delete piles.first(); // removes itself
    }
    disconnect();
    if ( d->m_solver_thread )
        d->m_solver_thread->finish();
    delete d->m_solver_thread;
    d->m_solver_thread = 0;
    delete d->m_solver;
    d->m_solver = 0;
    qDeleteAll( d->undoList );
    qDeleteAll( d->redoList );

    delete d;
}


// ----------------------------------------------------------------


void DealerScene::hint()
{
    kDebug() << waiting() << d->stateTimer->isActive();
    if ( waiting() && d->stateTimer->isActive() ) {
        QTimer::singleShot( 150, this, SLOT( hint() ) );
        return;
    }

    d->autoDropTimer->stop();

    setWaiting( true );

    unmarkAll();
    clearHints();
    kDebug(11111) << "hint" << d->winMoves.count();
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
            kDebug(11111) << "win move" << mh->pile()->objectName() << " " << mh->card()->rank() << " " << mh->card()->suit() << " " << mh->priority();
        }
        getHints();
    } else
        getHints();

    for (HintList::ConstIterator it = hints.constBegin(); it != hints.constEnd(); ++it)
        mark((*it)->card());
    clearHints();

    setWaiting( false );
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

    kDebug(11111) << "getSolverHints";
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

    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        if ((*it)->target())
            continue;

        Pile *store = *it;
        if (store->isEmpty())
            continue;
//        kDebug(11111) << "trying" << store->top()->name();

        CardList cards = store->cards();
        while (cards.count() && !cards.first()->realFace()) cards.erase(cards.begin());

        CardList::Iterator iti = cards.begin();
        while (iti != cards.end())
        {
            if (store->legalRemove(*iti)) {
//                kDebug(11111) << "could remove" << (*iti)->name();
                for (PileList::Iterator pit = piles.begin(); pit != piles.end(); ++pit)
                {
                    Pile *dest = *pit;
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
    qDeleteAll( hints );
    hints.clear();
}

void DealerScene::newHint(MoveHint *mh)
{
    //kDebug() << ( void* )mh << mh->pile()->objectName() <<
    //mh->card()->rank() << mh->card()->suit() << mh->priority();
    hints.append(mh);
}

bool DealerScene::isMoving(Card *c) const
{
    return movingCards.indexOf(c) != -1;
}

void DealerScene::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
    if (movingCards.isEmpty())
        return;

    moved = true;

    for (CardList::Iterator it = movingCards.begin(); it != movingCards.end(); ++it)
    {
        (*it)->setPos( ( *it )->pos() + e->scenePos() - moving_start );
    }

    // TODO some caching of the results
    unmarkAll();

    Pile * dropPile = targetPile();
    if (dropPile) {
        if (dropPile->isEmpty()) {
            dropPile->setHighlighted(true);
            marked.append(dropPile);
        } else {
            mark(dropPile->top());
        }
    }
    moving_start = e->scenePos();
}

void DealerScene::mark(Card *c)
{
    c->setHighlighted(true);
    if (!marked.contains(c))
        marked.append(c);
}

void DealerScene::unmarkAll()
{
    for (QList<QGraphicsItem *>::Iterator it = marked.begin(); it != marked.end(); ++it)
    {
        Card *c = dynamic_cast<Card*>( *it );
        if ( c )
            c->setHighlighted(false);
        Pile *p = dynamic_cast<Pile*>( *it );
        if ( p )
            p->setHighlighted(false);
    }
    marked.clear();
}

void DealerScene::mousePressEvent( QGraphicsSceneMouseEvent * e )
{
    // don't allow manual moves while automoves are going on
    if ( waiting() )
        return;

    unmarkAll();
    stopDemo();

    QList<QGraphicsItem *> list = items( e->scenePos() );

    //kDebug(11111) << gettime() << "mouse pressed" << list.count() << " " << items().count() << " " << e->scenePos().x() << " " << e->scenePos().y();
    moved = false;

    if (!list.count())
        return;

    QGraphicsScene::mousePressEvent( e );

    if (e->button() == Qt::LeftButton) {
        if (list.first()->type() == QGraphicsItem::UserType + DealerScene::CardTypeId ) {
            Card *c = dynamic_cast<Card*>(list.first());
            assert(c);
            CardList mycards = c->source()->cardPressed(c);
            for (CardList::Iterator it = mycards.begin(); it != mycards.end(); ++it)
                (*it)->stopAnimation();
            movingCards = mycards;
            moving_start = e->scenePos();
        }
        return;
    }

    if (e->button() == Qt::RightButton) {
        if (list.first()->type() == QGraphicsItem::UserType + DealerScene::CardTypeId ) {
            Card *preview = dynamic_cast<Card*>(list.first());
            assert(preview);
            if (!preview->animated() && !isMoving(preview))
            {
                if ( preview == preview->source()->top() )
                    mouseDoubleClickEvent( e ); // see bug #151921
                else
                    preview->getUp();
            }
        }
        return;
    }

    // if it's nothing else, we move the cards back
    mouseReleaseEvent(e);

}

class Hit {
public:
    Pile *source;
    QRectF intersect;
    bool top;
};
typedef QList<Hit> HitList;

void DealerScene::startNew()
{
    if ( waiting() )
    {
        QTimer::singleShot( 100, this, SLOT( startNew() ) );
        return;
    }

    if (!d->_won)
        countLoss();
    d->_won = false;
    _waiting = 0;
    d->_gameRecorded=false;
    delete d->wonItem;
    d->wonItem = 0;
    d->gothelp = false;
    qDeleteAll(d->undoList);
    d->undoList.clear();
    qDeleteAll(d->redoList);
    d->redoList.clear();
    d->toldAboutLostGame = false;
    d->toldAboutWonGame = false;
    emit updateMoves();

    stopDemo();
    kDebug(11111) << gettime() << "startNew unmarkAll\n";
    unmarkAll();
    kDebug(11111) << "startNew setAnimated(false)\n";
    QList<QGraphicsItem *> list = items();
    for (QList<QGraphicsItem *>::Iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId )
        {
            Card *c = static_cast<Card*>(*it);
            c->disconnect();
            c->stopAnimation();
        }
    }
    emit undoPossible(false);
    d->initialDeal = true;
    kDebug(11111) << "startNew restart\n";
    restart();
    takeState();
    update();
}

void DealerScene::showWonMessage()
{
    kDebug(11111) << "showWonMessage" << waiting();
    emit undoPossible( false );

    updateWonItem();

    emit demoPossible( false );
    emit hintPossible( false );
    emit redealPossible( false );
    // emit undoPossible(false); // technically it's possible but cheating :)
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
    if ( !d->wonItem || abs( d->wonItem->boundingRect().width() - boxWidth ) > 20 )
    {
        if ( !d->wonItem )
        {
            d->wonItem  = new QGraphicsPixmapItem( 0, this );
            d->wonItem->setZValue( 2000 );
        }

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
        p.setPen( Qt::white );
        p.drawText( contentsRect, Qt::AlignCenter, text );
        p.end();

        d->wonItem->setPixmap( pix );
    }

    d->wonItem->setPos( QPointF( ( width() - d->wonItem->sceneBoundingRect().width() ) / 2,
                                 ( height() - d->wonItem->sceneBoundingRect().height() ) / 2 ) );
}

void DealerScene::mouseReleaseEvent( QGraphicsSceneMouseEvent *e )
{
    QGraphicsScene::mouseReleaseEvent( e );

    if (!moved) {
        if (!movingCards.isEmpty()) {
            movingCards.first()->source()->moveCardsBack(movingCards);
            movingCards.clear();
        }
        QList<QGraphicsItem *> list = items(e->scenePos());
        if (list.isEmpty())
            return;
        QList<QGraphicsItem *>::Iterator it = list.begin();
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId ) {
            Card *c = dynamic_cast<Card*>(*it);
            assert(c);
            if (!c->animated()) {
                if ( cardClicked(c) ) {
                    countGame();
                }
                takeState();
                eraseRedo();
            }
            return;
        }
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::PileTypeId ) {
            Pile *c = dynamic_cast<Pile*>(*it);
            assert(c);
            pileClicked(c);
            takeState();
            eraseRedo();
            return;
        }
    }

    if (movingCards.isEmpty())
        return;

    unmarkAll();

    Pile * destination = targetPile();
    if (destination) {
        Card *c = static_cast<Card*>(movingCards.first());
        assert(c);
        countGame();
        c->source()->moveCards(movingCards, destination);
        takeState();
        eraseRedo();
    }
    else
    {
        movingCards.first()->source()->moveCardsBack(movingCards);
    }
    movingCards.clear();
}

Pile * DealerScene::targetPile()
{
    QList<QGraphicsItem *> list = collidingItems( movingCards.first() );
    HitList sources;

    for (QList<QGraphicsItem *>::Iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId ) {
            Card *c = dynamic_cast<Card*>(*it);
            assert(c);
            if (!c->isFaceUp())
                continue;
            if (c->source() == movingCards.first()->source())
                continue;
            Hit t;
            t.source = c->source();
            t.intersect = c->sceneBoundingRect().intersect(movingCards.first()->sceneBoundingRect());
            t.top = (c == c->source()->top());

            bool found = false;
            for (HitList::Iterator hi = sources.begin(); hi != sources.end(); ++hi)
            {
                if ((*hi).source == c->source()) {
                    found = true;
                    if ((*hi).intersect.width() * (*hi).intersect.height() >
                        t.intersect.width() * t.intersect.height())
                    {
                        (*hi).intersect = t.intersect;
                        (*hi).top |= t.top;
                    }
                }
            }
            if (found)
                continue;

            sources.append(t);
        } else {
            if ((*it)->type() == QGraphicsItem::UserType + DealerScene::PileTypeId ) {
                Pile *p = static_cast<Pile*>(*it);
                if (p->isEmpty())
                {
                    Hit t;
                    t.source = p;
                    t.intersect = p->sceneBoundingRect().intersect(movingCards.first()->sceneBoundingRect());
                    t.top = true;
                    sources.append(t);
                }
            } else {
                kDebug(11111) << "unknown object" << *it << " " << (*it)->type();
            }
        }
    }

    for (HitList::Iterator it = sources.begin(); it != sources.end(); )
    {
        if (!(*it).source->legalAdd(movingCards))
            it = sources.erase(it);
        else
            ++it;
    }

    if (!sources.isEmpty()) {
        HitList::Iterator best = sources.begin();
        HitList::Iterator it = best;
        for (++it; it != sources.end(); ++it )
        {
            if ((*it).intersect.width() * (*it).intersect.height() >
                (*best).intersect.width() * (*best).intersect.height()
                || ((*it).top && !(*best).top))
            {
                best = it;
            }
        }
        return (*best).source;
    } else {
        return 0;
    }
}

void DealerScene::mouseDoubleClickEvent( QGraphicsSceneMouseEvent *e )
{
    stopDemo();
    unmarkAll();
    if (waiting())
        return;

    if (!movingCards.isEmpty()) {
        movingCards.first()->source()->moveCardsBack(movingCards);
        movingCards.clear();
    }
    QList<QGraphicsItem *> list = items(e->scenePos());
    kDebug(11111) << "mouseDoubleClickEvent" << list;
    if (list.isEmpty())
        return;
    QList<QGraphicsItem *>::Iterator it = list.begin();
    if ((*it)->type() != QGraphicsItem::UserType + DealerScene::CardTypeId )
        return;
    Card *c = dynamic_cast<Card*>(*it);
    assert(c);
    c->stopAnimation();
    kDebug(11111) << "card" << c->rank() << " " << c->suit();
    if ( cardDblClicked(c) ) {
        countGame();
    }
    takeState();
    eraseRedo();
}

bool DealerScene::cardClicked(Card *c) {
    return c->source()->cardClicked(c);
}

void DealerScene::pileClicked(Pile *c) {
    c->cardClicked(0);
}

bool DealerScene::cardDblClicked(Card *c)
{
    if (c->source()->cardDblClicked(c))
        return true;

    if (c->animated())
        return false;

    if ( !c->source()->legalRemove( c ) )
        return false;

    if (c == c->source()->top() && c->realFace()) {
        Pile *tgt = findTarget(c);
        if (tgt) {
            CardList empty;
            empty.append(c);
            c->source()->moveCards(empty, tgt);
            return true;
        }
    }
    return false;
}


State *DealerScene::getState()
{
    QList<QGraphicsItem *> list = items();
    State * st = new State;

    for (QList<QGraphicsItem *>::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it)
    {
       if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId ) {
           Card *c = dynamic_cast<Card*>(*it);
           assert(c);
           CardState s;
           s.it = c;
           s.source = c->source();
           if (!s.source) {
               kDebug(11111) << c->rank() << " " << c->suit() << "has no parent\n";
               assert(false);
           }
           s.source_index = c->source()->indexOf(c);
           s.z = c->realZ();
           s.faceup = c->realFace();
           s.tookdown = c->takenDown();
           st->cards.append(s);
       }
    }
    qSort(st->cards);

    // Game specific information
    st->gameData = getGameState( );

    return st;
}

void DealerScene::setState(State *st)
{
    kDebug(11111) << gettime() << "setState\n";
    CardStateList * n = &st->cards;
    QList<QGraphicsItem *> list = items();

    for (QList<QGraphicsItem *>::Iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::PileTypeId )
        {
            Pile *p = dynamic_cast<Pile*>(*it);
            assert(p);
            CardList cards = p->cards();
            for (CardList::Iterator it = cards.begin(); it != cards.end(); ++it)
                (*it)->setTakenDown(p->target());
            p->clear();
        }
    }

    for (CardStateList::ConstIterator it = n->constBegin(); it != n->constEnd(); ++it)
    {
        Card *c = (*it).it;
        //c->stopAnimation();
        CardState s = *it;
        // kDebug() << "c" << c->name() << " " << s.source->objectName() << " " << s.faceup;
        bool target = c->takenDown(); // abused
        c->turn(s.faceup);
        s.source->add(c, s.source_index);
        c->setVisible(s.source->isVisible());
        c->setZValue(s.z);
        c->setTakenDown(s.tookdown || (target && !s.source->target()));
    }

    for (PileList::ConstIterator it = piles.constBegin(); it != piles.constEnd(); ++it)
    {
        ( *it )->relayoutCards();
    }

    for (CardStateList::ConstIterator it = n->constBegin(); it != n->constEnd(); ++it)
    {
        Card *c = (*it).it;
        c->stopAnimation();
    }

    // restore game-specific information
    setGameState( st->gameData );

    delete st;
}

Pile *DealerScene::findTarget(Card *c)
{
    if (!c)
        return 0;

    CardList empty;
    empty.append(c);
    for (PileList::ConstIterator it = piles.constBegin(); it != piles.constEnd(); ++it)
    {
        if (!(*it)->target())
            continue;
        if ((*it)->legalAdd(empty))
            return *it;
    }
    return 0;
}

void DealerScene::setWaiting(bool w)
{
    //kDebug(11111) << "setWaiting" << w << " " << _waiting;
    assert( _waiting > 0 || w );

    if (w)
        _waiting++;
    else if ( _waiting > 0 )
        _waiting--;
    //kDebug(11111) << "setWaiting" << w << " " << _waiting;
}

void DealerScene::setAutoDropEnabled(bool a)
{
    _autodrop = a;
    d->autoDropTimer->start( TIME_BETWEEN_MOVES );
}

void DealerScene::slotAutoDrop()
{
    d->m_autoDropOnce = true;
    d->autoDropTimer->start( TIME_BETWEEN_MOVES );
}

void DealerScene::setSolverEnabled(bool a)
{
    _usesolver = a;
    if(_usesolver)
        startSolver();
}

bool DealerScene::startAutoDrop()
{
    if (!autoDrop() && !d->m_autoDropOnce)
        return false;

    if (!movingCards.isEmpty() || waiting() ) {
        d->autoDropTimer->start( speedUpTime( TIME_BETWEEN_MOVES ) );
        return true;
    }

    QList<QGraphicsItem *> list = items();

    //kDebug( 11111 ) << "startAutoDrop\n";
    for (QList<QGraphicsItem *>::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it)
    {
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId ) {
            Card *c = static_cast<Card*>(*it);
            if (c->animated()) {
                d->autoDropTimer->start( speedUpTime( TIME_BETWEEN_MOVES ) );
                //kDebug(11111) << "animation still going on\n";
                return true;
            }
        }
    }

    kDebug(11111) << gettime() << "startAutoDrop \n";

    clearHints();
    getHints();
    for (HintList::ConstIterator it = hints.constBegin(); it != hints.constEnd(); ++it) {
        MoveHint *mh = *it;
        if (mh->pile() && mh->pile()->target() && mh->priority() > 120 && !mh->card()->takenDown()) {
            //setWaiting(true);
            Card *t = mh->card();
            CardList cards = mh->card()->source()->cards();
            while (cards.count() && cards.first() != t)
                cards.erase(cards.begin());
            QList<double> xs, ys;
            for ( CardList::Iterator it2 = cards.begin(); it2 != cards.end(); ++it2 )
            {
                QPointF p = ( *it2 )->pos();
                xs.append( p.x() );
                ys.append( p.y() );
            }

            double z = mh->pile()->zValue() + 1;
            if ( mh->pile()->top() )
                z = mh->pile()->top()->zValue() + 1;
            t->source()->moveCards(cards, mh->pile());
            int count = 0;

            if ( !cards.isEmpty() )
                unmarkAll();

            for ( CardList::Iterator it2 = cards.begin(); it2 != cards.end(); ++it2 )
            {
                Card *t = *it2;
                t->stopAnimation();
                t->turn(true);
                t->setPos(QPointF( xs.first(), ys.first()) );
                t->setZValue( z );
                z = z + 1;
                xs.removeFirst();
                ys.removeFirst();
                t->moveTo(t->source()->x(), t->source()->y(), t->zValue(),
                          speedUpTime( DURATION_AUTODROP + count++ * DURATION_AUTODROP / 10 ) );
                if ( t->animated() )
                {
                    setWaiting( true );
                    connect(t, SIGNAL(stoped(Card*)), SLOT(waitForAutoDrop(Card*)));
                }
            }
            d->m_autoDropFactor *= 0.8;
            return true;
        }
    }
    clearHints();
    d->m_autoDropFactor = 1;
    d->m_autoDropOnce = false;

    return false;
}

int DealerScene::speedUpTime( int delay ) const
{
    return qMax( 1, qRound( delay * d->m_autoDropFactor ) );
}

void DealerScene::stopAndRestartSolver()
{
    if ( d->toldAboutLostGame || d->toldAboutWonGame ) // who cares?
        return;

    kDebug(11111) << "stopAndRestartSolver\n";
    if ( d->m_solver_thread && d->m_solver_thread->isRunning() )
    {
        d->m_solver_thread->finish();
        d->solverMutex.unlock();
    }

    QList<QGraphicsItem *> list = items();
    for (QList<QGraphicsItem *>::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it)
    {
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId ) {
            Card *c = static_cast<Card*>(*it);
            if (c->animated()) {
                startSolver();
                //kDebug(11111) << "animation still going on\n";
                return;
            }
        }
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
    kDebug(11111) << gettime() << "start thread\n";
    d->winMoves.clear();
    emit gameSolverStart();
    if ( !d->m_solver_thread ) {
        d->m_solver_thread = new SolverThread( d->m_solver );
        connect( d->m_solver_thread, SIGNAL( finished() ), this, SLOT( slotSolverFinished() ));
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

    kDebug(11111) << gettime() << "stop thread" << ret;
    switch (  ret )
    {
    case Solver::WIN:
        d->winMoves = d->m_solver->winMoves;
        d->solverMutex.unlock();
        emit gameSolverWon();
        break;
    case Solver::NOSOL:
        d->solverMutex.unlock();
        emit gameSolverLost();
        break;
    case Solver::FAIL:
        d->solverMutex.unlock();
        emit gameSolverUnknown();
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

void DealerScene::waitForAutoDrop(Card * c) {
    //kDebug(11111) << "waitForAutoDrop" << c->name();
    setWaiting(false);
    c->disconnect();
    c->stopAnimation(); // should be a no-op
    takeState();
    eraseRedo();
}

void DealerScene::waitForWonAnim(Card *c)
{
    //kDebug() << "waitForWonAnim" << c->name() << " " << waiting();
    if ( !waiting() )
        return;

    c->setVisible( false ); // hide it
    c->disconnect( this, SLOT( waitForWonAnim( Card* ) ) );

    setWaiting(false);

    if ( !waiting() )
    {
        stopDemo();
        emit gameWon(d->gothelp);
    }
}

long DealerScene::gameNumber() const
{
    return gamenumber;
}

void DealerScene::rescale(bool onlypiles)
{
    if ( !onlypiles )
        Deck::deck()->update();
    for (PileList::ConstIterator it = piles.constBegin(); it != piles.constEnd(); ++it)
    {
        ( *it )->rescale();
    }
}

void DealerScene::setGameNumber(long gmn)
{
    // Deal in the range of 1 to INT_MAX.
    gamenumber = ((gmn < 1) ? 1 : ((gmn > 0x7FFFFFFF) ? 0x7FFFFFFF : gmn));
    qDeleteAll(d->undoList);
    d->undoList.clear();
    qDeleteAll(d->redoList);
    d->redoList.clear();
}

void DealerScene::addPile(Pile *p)
{
    if ( p->scene() )
    {
        // might be this
        p->dscene()->removePile( p );
    }
    piles.append(p);
}

void DealerScene::removePile(Pile *p)
{
    piles.removeAll(p);
}

void DealerScene::stopDemo()
{
    kDebug(11111) << gettime() << "stopDemo" << waiting();

    if (waiting()) {
        d->stop_demo_next = true;
        return;
    } else d->stop_demo_next = false;

    QList<QGraphicsItem *> list = items();
    for (QList<QGraphicsItem *>::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it)
    {
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId ) {
            Card *c = static_cast<Card*>(*it);
            c->stopAnimation();
        }
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

class CardPtr
{
 public:
    Card *ptr;
};

bool operator <(const CardPtr &p1, const CardPtr &p2)
{
    return ( p1.ptr->zValue() < p2.ptr->zValue() );
}

void DealerScene::won()
{
    kDebug(11111) << "won" << waiting();
    if (d->_won)
        return;
    d->_won = true;

    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        ( *it )->relayoutCards(); // stop the timer
    }

    // update score, 'win' in demo mode also counts (keep it that way?)
    KConfigGroup kc(KGlobal::config(), scores_group);
    unsigned int n = kc.readEntry(QString("won%1").arg(d->_id),0) + 1;
    kc.writeEntry(QString("won%1").arg(d->_id),n);
    n = kc.readEntry(QString("winstreak%1").arg(d->_id),0) + 1;
    kc.writeEntry(QString("winstreak%1").arg(d->_id),n);
    unsigned int m = kc.readEntry(QString("maxwinstreak%1").arg(d->_id),0);
    if (n>m)
        kc.writeEntry(QString("maxwinstreak%1").arg(d->_id),n);
    kc.writeEntry(QString("loosestreak%1").arg(d->_id),0);
    kc.sync();

    // sort cards by increasing z
    QList<QGraphicsItem *> list = items();
    QList<CardPtr> cards;
    for (QList<QGraphicsItem *>::ConstIterator it=list.constBegin(); it!=list.constEnd(); ++it)
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId ) {
            CardPtr p;
            p.ptr = dynamic_cast<Card*>(*it);
            assert(p.ptr);
            cards.push_back(p);
        }
    qSort(cards);

    // disperse the cards everywhere
    QRectF can(0, 0, width(), height());
    QListIterator<CardPtr> cit(cards);
    while (cit.hasNext()) {
        CardPtr card = cit.next();
        card.ptr->turn(true);
        QRectF p = card.ptr->sceneBoundingRect();
        p.moveTo( 0, 0 );
        qreal x, y;
        do {
            x = 3*width()/2 - KRandom::random() % int(width() * 2);
            y = 3*height()/2 - (KRandom::random() % int(height() * 2));
            p.moveTopLeft(QPointF(x, y));
        } while (can.intersects(p));

	card.ptr->moveTo( x, y, 0, 1200);
        connect(card.ptr, SIGNAL(stoped(Card*)), SLOT(waitForWonAnim(Card*)));
        setWaiting(true);
    }
}

MoveHint *DealerScene::chooseHint()
{
    kDebug(11111) << "chooseHint " << d->winMoves.count();
    if ( d->winMoves.count() )
    {
        MOVE m = d->winMoves.takeFirst();
        if ( m.totype == O_Type )
            fprintf( stderr, "move from %d out (at %d) Prio: %d\n", m.from,
                     m.turn_index, m.pri );
        else
            fprintf( stderr, "move from %d to %d (%d) Prio: %d\n", m.from, m.to,
                     m.turn_index, m.pri );
        MoveHint *mh = solver()->translateMove( m );

        if ( mh )
            hints.append( mh );
        return mh;
    }

    if (hints.isEmpty())
        return 0;

    qSort( hints.begin(), hints.end() );
    return hints[0];
}

void DealerScene::demo()
{
    kDebug(11111) << "demo" << waiting() << d->stateTimer->isActive();
    if ( waiting() || d->stateTimer->isActive() )
        return;

    if (d->stop_demo_next) {
        stopDemo();
        return;
    }
    d->stop_demo_next = false;
    d->demo_active = true;
    d->gothelp = true;
    countGame();
    unmarkAll();
    clearHints();
    getHints();
    d->demotimer->stop();

    MoveHint *mh = chooseHint();
    if (mh) {
        kDebug(11111) << "moveFrom" << mh->card()->source()->objectName();
        myassert(mh->card()->source() == Deck::deck() ||
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

        qreal *oldcoords = new qreal[2*empty.count()];
        int i = 0;

        for (CardList::Iterator it = empty.begin(); it != empty.end(); ++it) {
            Card *t = *it;
            t->stopAnimation();
            t->turn(true);
            oldcoords[i++] = t->realX();
            oldcoords[i++] = t->realY();
        }

        assert(mh->card());
        assert(mh->card()->source());
        assert(mh->pile());
        assert(mh->card()->source() != mh->pile());
        kDebug(11111) << "moveTo" << mh->pile()->objectName();
        assert(mh->pile()->target() || mh->pile()->legalAdd(empty));

        mh->card()->source()->moveCards(empty, mh->pile());

        i = 0;

        for (CardList::Iterator it = empty.begin(); it != empty.end(); ++it) {
            Card *t = *it;
            qreal x1 = oldcoords[i++];
            qreal y1 = oldcoords[i++];
            qreal x2 = t->realX();
            qreal y2 = t->realY();
            t->stopAnimation();
            t->setPos(QPointF( x1, y1) );
            t->moveTo(x2, y2, t->zValue(), DURATION_DEMO);
        }

        delete [] oldcoords;

        newDemoMove(mh->card());

    } else {
        kDebug(11111) << "demoNewCards";
        Card *t = demoNewCards();
        if (t) {
            newDemoMove(t);
        } else if (isGameWon()) {
            emit gameWon(true);
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

Card *DealerScene::demoNewCards()
{
    return 0;
}

void DealerScene::newDemoMove(Card *m)
{
    kDebug(11111) << "newDemoMove" << m->rank() << " " << m->suit();
    if ( m->animated() )
    {
        connect(m, SIGNAL(stoped(Card*)), SLOT(waitForDemo(Card*)));
        setWaiting( true );
    } else
        waitForDemo( 0 );
}

void DealerScene::waitForDemo(Card *t)
{
    if ( t )
    {
        kDebug(11111) << "waitForDemo" << t->rank() << " " << t->suit();
        t->disconnect(this, SLOT( waitForDemo( Card* ) ) );
        setWaiting( false );
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
    for (PileList::ConstIterator it = piles.constBegin(); it != piles.constEnd(); ++it)
    {
        if (!(*it)->target() && !(*it)->isEmpty())
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

bool DealerScene::checkRemove( int, const Pile *, const Card *) const {
    return true;
}

bool DealerScene::checkAdd( int, const Pile *, const CardList&) const {
    return true;
}

void DealerScene::countGame()
{
    if ( !d->_gameRecorded ) {
        kDebug(11111) << "counting game as played.";
        KConfigGroup kc(KGlobal::config(), scores_group);
        unsigned int Total = kc.readEntry(QString("total%1").arg(d->_id),0);
        ++Total;
        kc.writeEntry(QString("total%1").arg(d->_id),Total);
        d->_gameRecorded = true;
	kc.sync();
    }
}

void DealerScene::countLoss()
{
    if ( d->_gameRecorded ) {
        // update score
        KConfigGroup kc(KGlobal::config(), scores_group);
        unsigned int n = kc.readEntry(QString("loosestreak%1").arg(d->_id),0) + 1;
        kc.writeEntry(QString("loosestreak%1").arg(d->_id),n);
        unsigned int m = kc.readEntry(QString("maxloosestreak%1").arg(d->_id),0);
        if (n>m)
            kc.writeEntry(QString("maxloosestreak%1").arg(d->_id),n);
        kc.writeEntry(QString("winstreak%1").arg(d->_id),0);
        kc.sync();
    }
}

QPointF DealerScene::maxPilePos() const
{
    QPointF maxp( 0, 0 );
    for (PileList::ConstIterator it = piles.constBegin(); it != piles.constEnd(); ++it)
    {
        // kDebug() << ( *it )->objectName() << " " <<  ( *it )->pilePos() << " " << ( *it )->reservedSpace();
        // we call fabs here, because reserved space of -50 means for a pile at -1x-1 means it wants at least
        // 49 all together. Even though the real logic should be more complex, it's enough for our current needs
        maxp = QPointF( qMax( qAbs( ( *it )->pilePos().x() + ( *it )->reservedSpace().width() ), maxp.x() ),
                        qMax( qAbs( ( *it )->pilePos().y() + ( *it )->reservedSpace().height() ), maxp.y() ) );
    }
    return maxp;
}

void DealerScene::relayoutPiles()
{
     if ( !views().isEmpty() ) {
        QGraphicsView *view = views().first();
        setSceneSize( view->viewport()->size() );
        view->resetCachedContent();
     }
}

void DealerScene::setSceneSize( const QSize &s )
{
    kDebug(11111) << "setSceneSize" << sceneRect() << s;
    setSceneRect( QRectF( 0, 0, s.width(), s.height() ) );
    d->hasScreenRect = true;

    QPointF defaultSceneRect = maxPilePos();

    kDebug(11111) << gettime() << "setSceneSize" << defaultSceneRect << " " << s;
    Q_ASSERT( cardMap::self()->wantedCardWidth() > 0 );
    Q_ASSERT( cardMap::self()->wantedCardHeight() > 0 );

    qreal scaleX = s.width() / cardMap::self()->wantedCardWidth() / ( defaultSceneRect.x() + 2 );
    qreal scaleY = s.height() / cardMap::self()->wantedCardHeight() / ( defaultSceneRect.y() + 2 );
    //kDebug() << "scales" << scaleY << scaleX;
    qreal n_scaleFactor = qMin(scaleX, scaleY);

    d->offx = 0;
    d->offy = 0; // just for completness
    if ( n_scaleFactor < scaleX )
    {
        d->offx = ( s.width() - ( ( defaultSceneRect.x() + 2 ) * n_scaleFactor ) * cardMap::self()->wantedCardWidth() ) / 2;
    }

    cardMap::self()->setWantedCardWidth( int( n_scaleFactor * 10 * cardMap::self()->wantedCardWidth() ) );

    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        Pile *p = *it;
        if ( p->pilePos().x() < 0 || p->pilePos().y() < 0 || d->offx > 0 )
            p->rescale();
        QSizeF ms( cardMap::self()->wantedCardWidth(),
                   cardMap::self()->wantedCardHeight() );
        //kDebug() << p->objectName() << p->reservedSpace() << p->pos() << ms;
        if ( p->reservedSpace().width() > 10 )
            ms.setWidth( s.width() - p->x() );
        if ( p->reservedSpace().height() > 10 && s.height() > p->y() )
            ms.setHeight( s.height() - p->y() );
        if ( p->reservedSpace().width() < 0 && p->x() > 0 )
            ms.setWidth( p->x() );
        Q_ASSERT( p->reservedSpace().height() > 0 ); // no such case yet

        //kDebug() << "setMaximalSpace" << ms << cardMap::self()->wantedCardHeight() << cardMap::self()->wantedCardWidth();
        if ( ms.width() < cardMap::self()->wantedCardWidth() - 0.2 )
            return;
        if ( ms.height() < cardMap::self()->wantedCardHeight() - 0.2 )
            return;

        p->setMaximalSpace( ms );
    }

    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        Pile *p = *it;
        if ( !p->isVisible() || ( p->reservedSpace().width() == 10 && p->reservedSpace().height() == 10 ) )
            continue;

        QRectF myRect( p->pos(), p->maximalSpace() );
        if ( p->reservedSpace().width() < 0 )
        {
            myRect.moveRight( p->x() + cardMap::self()->wantedCardWidth() );
            myRect.moveLeft( p->x() - p->maximalSpace().width() );
        }

        //kDebug() << p->objectName() << " " << p->spread() << " " << myRect;

        for ( PileList::ConstIterator it2 = piles.constBegin(); it2 != piles.constEnd(); ++it2 )
        {
            if ( *it2 == p || !( *it2 )->isVisible() )
                continue;

            QRectF pileRect( (*it2)->pos(), (*it2)->maximalSpace() );
            if ( (*it2)->reservedSpace().width() < 0 )
                pileRect.moveRight( (*it2)->x() );
            if ( (*it2)->reservedSpace().height() < 0 )
                pileRect.moveBottom( (*it2)->y() );


            //kDebug() << "compa" << p->objectName() << myRect << ( *it2 )->objectName() << pileRect << myRect.intersects( pileRect );
            // if the growing pile intersects with another pile, we need to solve the conflict
            if ( myRect.intersects( pileRect ) )
            {
                //kDebug() << "compa" << p->objectName() << myRect << ( *it2 )->objectName() << pileRect << myRect.intersects( pileRect );
                QSizeF pms = ( *it2 )->maximalSpace();

                if ( p->reservedSpace().width() != 10 )
                {
                    if ( ( *it2 )->reservedSpace().height() != 10 )
                    {
                        Q_ASSERT( ( *it2 )->reservedSpace().height() > 0 );
                        // if it's growing too, we win
                        pms.setHeight( qMin( myRect.top() - ( *it2 )->y() - 1, pms.height() ) );
                        //kDebug() << "1. reduced height of" << ( *it2 )->objectName();
                    } else // if it's fixed height, we loose
                        if ( p->reservedSpace().width() < 0 ) {
                            // this isn't made for two piles one from left and one from right both growing
                            Q_ASSERT( ( *it2 )->reservedSpace().width() == 10 );
                            myRect.setLeft( ( *it2 )->x() + cardMap::self()->wantedCardWidth() + 1);
                            //kDebug() << "2. reduced width of" << p->objectName();
                        } else {
                            myRect.setRight( ( *it2 )->x() - 1 );
                            //kDebug() << "3. reduced width of" << p->objectName();
                        }
                }

                if ( p->reservedSpace().height() != 10 )
                {
                    if ( ( *it2 )->reservedSpace().width() != 10 )
                    {
                        Q_ASSERT( ( *it2 )->reservedSpace().height() > 0 );
                        // if it's growing too, we win
                        pms.setWidth( qMin( myRect.right() - ( *it2 )->x() - 1, pms.width() ) );
                        //kDebug() << "4. reduced width of" << ( *it2 )->objectName();
                    } else // if it's fixed height, we loose
                        if ( p->reservedSpace().height() < 0 ) {
                            // this isn't made for two piles one from left and one from right both growing
                            Q_ASSERT( ( *it2 )->reservedSpace().height() == 10 );
                            Q_ASSERT( false ); // TODO
                            myRect.setLeft( ( *it2 )->x() + cardMap::self()->wantedCardWidth() + 1);
                            //kDebug() << "5. reduced height of" << p->objectName();
                        } else if ( ( *it2 )->y() >= 1  ) {
                            myRect.setBottom( ( *it2 )->y() - 1 );
                            //kDebug() << "6. reduced height of" << p->objectName() << (*it2)->y() - 1 << myRect;
                        }
                }

                kDebug() << pms  << cardMap::self()->wantedCardWidth()  << cardMap::self()->wantedCardHeight();
                Q_ASSERT( pms.width() >= cardMap::self()->wantedCardWidth() - 0.1 );
                Q_ASSERT( pms.height() >= cardMap::self()->wantedCardHeight() - 0.1 );
                ( *it2 )->setMaximalSpace( pms );
            }
        }
        kDebug() << myRect << cardMap::self()->wantedCardHeight();
        Q_ASSERT( myRect.width() >= cardMap::self()->wantedCardWidth() - 0.1 );
        Q_ASSERT( myRect.height() >= cardMap::self()->wantedCardHeight() - 0.1 );

        p->setMaximalSpace( myRect.size() );
    }

    if ( d->wonItem )
        updateWonItem();

    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        ( *it )->relayoutCards();
    }
}

void DealerScene::setGameId(int id) { d->_id = id; }
int DealerScene::gameId() const { return d->_id; }

void DealerScene::setActions(int actions) { d->myActions = actions; }
int DealerScene::actions() const { return d->myActions; }

Solver *DealerScene::solver() const { return d->m_solver; }

int DealerScene::neededFutureMoves() const { return d->neededFutureMoves; }
void DealerScene::setNeededFutureMoves( int i ) { d->neededFutureMoves = i; }

qreal DealerScene::offsetX() const { return d->offx; }
qreal DealerScene::offsetY() const { return d->offy; }

void DealerScene::drawBackground ( QPainter * painter, const QRectF & rect )
{
    kDebug(11111) << "drawBackground" << rect << d->hasScreenRect;
    if ( !d->hasScreenRect )
        return;

    painter->drawPixmap( 0, 0, Render::renderElement( "background", sceneRect().size().toSize() ) );
}

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
    QList<QGraphicsItem *> list = items();
    for (int i = 0; i < list.size(); ++i) {
        QGraphicsItem *item = list.at(i);
        assert( item->zValue() >= 0 );
        maxz = qMax( maxz, int( item->zValue() ) );
    }
    QStyleOptionGraphicsItem options;
    QPainter p( device );

    for ( int z = 0; z <= maxz; ++z )
        for (int i = 0; i < list.size(); ++i)
        {
            QGraphicsItem *item = list.at(i);
            if ( !item->isVisible() || item->zValue() != z )
                continue;

            if (item->type() == QGraphicsItem::UserType + DealerScene::CardTypeId )
            {

            }
            else if (item->type() == QGraphicsItem::UserType + DealerScene::PileTypeId )
            {
                QGraphicsPixmapItem *pitem = dynamic_cast<QGraphicsPixmapItem*>( item );
                assert( pitem );
                p.save();
                p.setTransform(item->deviceTransform(p.worldTransform()), false);
                //item->paint( &p, &options );
                p.drawPixmap( 0, 0, pitem->pixmap() );
                p.restore();
                continue;
            } else
                assert( false );

            p.save();
            p.setTransform(item->deviceTransform(p.worldTransform()), false);
            item->paint( &p, &options );
            p.restore();
        }
}

void DealerScene::mapOldId(int) {}

#include "dealer.moc"
