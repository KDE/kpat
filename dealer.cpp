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

#include "cardmaps.h"
#include "deck.h"
#include "render.h"
#include "speeds.h"
#include "version.h"
#include "view.h"
#include "patsolve/patsolve.h"

#include <KConfigGroup>
#include <KDebug>
#include <KLocalizedString>
#include <KMessageBox>
#include <KRandom>
#include <KSharedConfig>
#include <KTemporaryFile>
#include <KIO/NetAccess>

#include <QtCore/QCoreApplication>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QGraphicsView>
#include <QtGui/QPainter>
#include <QtGui/QStyleOptionGraphicsItem>
#include <QtXml/QDomDocument>

#include <cassert>


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
//                      class DealerInfoList

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
        kDebug(11111) << "won\n";
    } else if ( ret == Solver::NOSOL )
    {
        kDebug(11111) << "lost\n";
    } else if ( ret == Solver::QUIT  || ret == Solver::MLIMIT )
    {
        kDebug(11111) << "quit\n";
    } else
        kDebug(11111) << "unknown\n";
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
    int loadedMoveCount;

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
    return d->loadedMoveCount + d->undoList.count();
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

    foreach(const QGraphicsItem * item, items())
    {
        const Card *c = qgraphicsitem_cast<const Card*>(item);
        if (c && c->animated())
        {
            d->stateTimer->start();
            return;
        }
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
        d->initialDeal = false;
        d->wasJustSaved = false;
        if (isGameWon()) {
            won();
            d->toldAboutWonGame = true;
            return;
        }
        else if ( !d->toldAboutWonGame && !d->toldAboutLostGame && isGameLost() ) {
            emit hintPossible( false );
            emit demoPossible( false );
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
                kDebug(11111) << "pile index" << p->index() << "taken twice\n";
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

    QDomNodeList piles = dealer.elementsByTagName("pile");

    CardList cards;
    foreach (QGraphicsItem *item, items())
    {
        Card *c = qgraphicsitem_cast<Card*>(item);
        if (c)
            cards.append(c);
    }

    Deck::deck()->collectAndShuffle();

    foreach (QGraphicsItem *item, items())
    {
        Pile *p = qgraphicsitem_cast<Pile*>(item);
        if (p)
        {
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

    emit updateMoves(getMoves());
    emit hintPossible(true);
    emit demoPossible(true);
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
    emit gameSolverReset();
}

void DealerScene::redo()
{
    unmarkAll();
    stopDemo();
    kDebug(11111) << "::redo" << d->redoList.count();
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
    emit gameSolverReset();
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
    d->offx = d->offy = 0;
    d->neededFutureMoves = 1;
    d->toldAboutLostGame = false;
    d->toldAboutWonGame = false;
    d->hasScreenRect = false;
    d->updateSolver = new QTimer(this);
    d->updateSolver->setInterval( 250 );
    d->updateSolver->setSingleShot( true );
    d->initialDeal = true;
    d->loadedMoveCount = 0;
    connect( d->updateSolver, SIGNAL(timeout()), SLOT(stopAndRestartSolver()) );

    d->demotimer = new QTimer(this);
    connect( d->demotimer, SIGNAL(timeout()), SLOT(demo()) );
    d->m_autoDropFactor = 1;
    d->m_autoDropOnce = false;
    connect( this, SIGNAL(gameWon(bool)), SLOT(showWonMessage()) );

    d->stateTimer = new QTimer( this );
    connect( d->stateTimer, SIGNAL(timeout()), this, SLOT(takeState()) );
    d->stateTimer->setInterval( 100 );
    d->stateTimer->setSingleShot( true );

    d->autoDropTimer = new QTimer( this );
    connect( d->autoDropTimer, SIGNAL(timeout()), this, SLOT(startAutoDrop()) );
    d->autoDropTimer->setSingleShot( true );

    d->wonItem  = new QGraphicsPixmapItem( 0, this );
    d->wonItem->setZValue( 2000 );
    d->wonItem->hide();
}

DealerScene::~DealerScene()
{
    kDebug(11111) << "~DealerScene";
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
    kDebug(11111) << waiting() << d->stateTimer->isActive();
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
    //kDebug(11111) << ( void* )mh << mh->pile()->objectName() <<
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

    if (!moved) {
        bool overCard = movingCards.first()->sceneBoundingRect().contains(e->scenePos());
        QPointF delta = e->scenePos() - moving_start;
        qreal distanceSquared = delta.x() * delta.x() + delta.y() * delta.y();
        // Ignore the move event until we've moved at least 4 pixels or the
        // cursor leaves the card.
        if (distanceSquared > 16.0 || !overCard) {
            moved = true;
            // If the cursor hasn't left the card, then continue the drag from
            // the current position, which makes it look smoother.
            if (overCard)
                moving_start = e->scenePos();
        }
    }

    if (moved) {
        foreach ( Card *card, movingCards )
            card->setPos( card->pos() + e->scenePos() - moving_start );
        moving_start = e->scenePos();

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
    }
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

    QGraphicsItem *topItem = itemAt( e->scenePos() );

    //kDebug(11111) << gettime() << "mouse pressed" << list.count() << " " << items().count() << " " << e->scenePos().x() << " " << e->scenePos().y();
    moved = false;

    if (!topItem)
        return;

    QGraphicsScene::mousePressEvent( e );

    if (e->button() == Qt::LeftButton) {
        Card *c = qgraphicsitem_cast<Card*>(topItem);
        if (c) {
            movingCards = c->source()->cardPressed(c);
            foreach (Card *card, movingCards)
                card->stopAnimation();
            moving_start = e->scenePos();
        }
        return;
    }

    if (e->button() == Qt::RightButton) {
        Card *preview = qgraphicsitem_cast<Card*>(topItem);
        if (preview && !preview->animated() && !isMoving(preview))
        {
            if ( preview == preview->source()->top() )
                mouseDoubleClickEvent( e ); // see bug #151921
            else
                preview->getUp();
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

    if ( waiting() )
    {
        QTimer::singleShot( 100, this, SLOT( startNew() ) );
        return;
    }

    d->_won = false;
    d->wonItem->hide();
    _waiting = 0;
    d->gothelp = false;
    qDeleteAll(d->undoList);
    d->undoList.clear();
    qDeleteAll(d->redoList);
    d->redoList.clear();
    d->toldAboutLostGame = false;
    d->toldAboutWonGame = false;
    d->wasJustSaved = false;
    d->loadedMoveCount = 0;

    stopDemo();
    kDebug(11111) << gettime() << "startNew unmarkAll\n";
    unmarkAll();

    foreach (QGraphicsItem *item, items())
    {
        Card *c = qgraphicsitem_cast<Card*>(item);
        if (c)
        {
            c->disconnect();
            c->stopAnimation();
        }
    }

    emit gameSolverReset();
    emit updateMoves( 0 );
    emit demoPossible( true );
    emit hintPossible( true );
    d->initialDeal = true;

    restart();
    takeState();
    update();
}

void DealerScene::showWonMessage()
{
    kDebug(11111) << "showWonMessage" << waiting();

    d->wonItem->show();
    updateWonItem();

    emit demoPossible( false );
    emit hintPossible( false );
    emit newCardsPossible( false );
    emit undoPossible( false ); // technically it's possible but cheating :)
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
    if ( abs( d->wonItem->boundingRect().width() - boxWidth ) > 20 )
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

        QGraphicsItem * topItem = itemAt(e->scenePos());
        if (!topItem)
            return;

        Card *c = qgraphicsitem_cast<Card*>(topItem);
        if (c) {
            if (!c->animated() && cardClicked(c) ) {
                considerGameStarted();
                takeState();
                eraseRedo();
            }
            return;
        }

        Pile *p = qgraphicsitem_cast<Pile*>(topItem);
        if (p) {
            if (pileClicked(p)) {
                considerGameStarted();
                takeState();
                eraseRedo();
            }
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
        considerGameStarted();
        c->source()->moveCards(movingCards, destination);
        takeState();
        eraseRedo();
    }
    else
    {
        movingCards.first()->source()->moveCardsBack(movingCards);
    }
    movingCards.clear();
    moved = false;
}

Pile * DealerScene::targetPile()
{
    HitList sources;
    foreach (QGraphicsItem *item, collidingItems( movingCards.first() ))
    {
        Card *c = qgraphicsitem_cast<Card*>(item);
        if (c)
        {
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
            Pile *p = qgraphicsitem_cast<Pile*>(item);
            if (p)
            {
                if (p->isEmpty())
                {
                    Hit t;
                    t.source = p;
                    t.intersect = p->sceneBoundingRect().intersect(movingCards.first()->sceneBoundingRect());
                    t.top = true;
                    sources.append(t);
                }
            } else {
                kDebug(11111) << "unknown object" << item << " " << item->type();
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
    if (d->demo_active) {
        stopDemo();
        return;
    }

    if (waiting())
        return;

    unmarkAll();

    if (!movingCards.isEmpty()) {
        movingCards.first()->source()->moveCardsBack(movingCards);
        movingCards.clear();
    }

    QGraphicsItem *topItem = itemAt(e->scenePos());
    Card *c = qgraphicsitem_cast<Card*>(topItem);
    if (c && cardDblClicked(c) ) {
        considerGameStarted();
        takeState();
        eraseRedo();
    }
}

bool DealerScene::cardClicked(Card *c) {
    return c->source()->cardClicked(c);
}

bool DealerScene::pileClicked(Pile *p)
{
    p->cardClicked(0);
    return false;
}

bool DealerScene::cardDblClicked(Card *c)
{
    if (c->source()->cardDblClicked(c))
        return true;

    if (c->animated())
        return false;

    if (c == c->source()->top()  && c->realFace() && c->source()->legalRemove( c )) {
        Pile *tgt = findTarget(c);
        if (tgt) {
            c->source()->moveCards(CardList() << c , tgt);
            return true;
        }
    }
    return false;
}


State *DealerScene::getState()
{
    State * st = new State;
    foreach (QGraphicsItem *item, items())
    {
       Card *c = qgraphicsitem_cast<Card*>(item);
       if (c) {
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

    foreach (QGraphicsItem *item, items())
    {
        Pile *p = qgraphicsitem_cast<Pile*>(item);
        if (p)
        {
            foreach (Card *c, p->cards())
                c->setTakenDown(p->target());
            p->clear();
        }
    }

    for (CardStateList::ConstIterator it = n->constBegin(); it != n->constEnd(); ++it)
    {
        Card *c = (*it).it;
        //c->stopAnimation();
        CardState s = *it;
        // kDebug(11111) << "c" << c->name() << " " << s.source->objectName() << " " << s.faceup;
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

    foreach (const QGraphicsItem *item, items())
    {
        const Card *c = qgraphicsitem_cast<const Card*>(item);
        if (c && c->animated())
        {
            d->autoDropTimer->start( speedUpTime( TIME_BETWEEN_MOVES ) );
            return true;
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
                    connect(t, SIGNAL(stopped(Card*)), SLOT(waitForAutoDrop(Card*)));
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

    foreach (const QGraphicsItem *item, items())
    {
        const Card *c =qgraphicsitem_cast<const Card*>(item);
        if (c && c->animated())
        {
            startSolver();
            return;
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
    //kDebug(11111) << "waitForWonAnim" << c->name() << " " << waiting();
    if ( !waiting() )
        return;

    c->setVisible( false ); // hide it
    c->disconnect( this, SLOT(waitForWonAnim(Card*)) );

    setWaiting(false);

    if ( !waiting() )
    {
        stopDemo();
        emit gameWon(d->gothelp);
    }
}

int DealerScene::gameNumber() const
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

void DealerScene::setGameNumber(int gmn)
{
    // Deal in the range of 1 to INT_MAX.
    gamenumber = qMax(1, gmn);
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

    recordGameStatistics();

    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        ( *it )->relayoutCards(); // stop the timer
    }

    QList<CardPtr> cards;
    foreach (QGraphicsItem *item, items())
    {
        Card *c = qgraphicsitem_cast<Card*>(item);
        if (c)
        {
            CardPtr p;
            p.ptr = c;
            cards.push_back(p);
        }
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
        connect(card.ptr, SIGNAL(stopped(Card*)), SLOT(waitForWonAnim(Card*)));
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
    considerGameStarted();
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
        Card *t = newCards();
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

Card *DealerScene::newCards()
{
    return 0;
}

void DealerScene::newDemoMove(Card *m)
{
    kDebug(11111) << "newDemoMove" << m->rank() << " " << m->suit();
    if ( m->animated() )
    {
        connect(m, SIGNAL(stopped(Card*)), SLOT(waitForDemo(Card*)));
        setWaiting( true );
    } else
        waitForDemo( 0 );
}

void DealerScene::waitForDemo(Card *t)
{
    if ( t )
    {
        kDebug(11111) << "waitForDemo" << t->rank() << " " << t->suit();
        t->disconnect(this, SLOT(waitForDemo(Card*)) );
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

void DealerScene::considerGameStarted()
{
    d->gameStarted = true;
}

void DealerScene::recordGameStatistics()
{
    // Don't record the game if it was never started, if it is unchanged since
    // it was last saved (allowing the user to close KPat after saving without
    // it recording a loss) or if it has already been recorded.
    if ( d->gameStarted && !d->wasJustSaved && !d->_gameRecorded )
    {
        QString totalPlayedKey = QString("total%1").arg( d->_id );
        QString wonKey = QString("won%1").arg( d->_id );
        QString winStreakKey = QString("winstreak%1").arg( d->_id );
        QString maxWinStreakKey = QString("maxwinstreak%1").arg( d->_id );
        QString loseStreakKey = QString("loosestreak%1").arg( d->_id );
        QString maxLoseStreakKey = QString("maxloosestreak%1").arg( d->_id );

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

QPointF DealerScene::maxPilePos() const
{
    QPointF maxp( 0, 0 );
    for (PileList::ConstIterator it = piles.constBegin(); it != piles.constEnd(); ++it)
    {
        // kDebug(11111) << ( *it )->objectName() << " " <<  ( *it )->pilePos() << " " << ( *it )->reservedSpace();
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

    qreal cardWidth = cardMap::self()->cardWidth();
    qreal cardHeight = cardMap::self()->cardHeight();

    kDebug(11111) << gettime() << "setSceneSize" << defaultSceneRect << " " << s;
    Q_ASSERT( cardWidth > 0 );
    Q_ASSERT( cardHeight > 0 );

    qreal scaleX = s.width() / cardWidth / ( defaultSceneRect.x() + 2 );
    qreal scaleY = s.height() / cardHeight / ( defaultSceneRect.y() + 2 );
    //kDebug(11111) << "scales" << scaleY << scaleX;
    qreal n_scaleFactor = qMin(scaleX, scaleY);

    d->offx = 0;
    d->offy = 0; // just for completness
    if ( n_scaleFactor < scaleX )
    {
        d->offx = ( s.width() - ( ( defaultSceneRect.x() + 2 ) * n_scaleFactor ) * cardWidth ) / 2;
    }

    cardMap::self()->setCardWidth( int( n_scaleFactor * 10 * cardWidth ) );

    cardWidth = cardMap::self()->cardWidth();
    cardHeight = cardMap::self()->cardHeight();

    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        Pile *p = *it;
        if ( p->pilePos().x() < 0 || p->pilePos().y() < 0 || d->offx > 0 )
            p->rescale();
        QSizeF ms( cardWidth, cardHeight );

        if ( p->reservedSpace().width() > 10 )
            ms.setWidth( s.width() - p->x() );
        if ( p->reservedSpace().height() > 10 && s.height() > p->y() )
            ms.setHeight( s.height() - p->y() );
        if ( p->reservedSpace().width() < 0 && p->x() > 0 )
            ms.setWidth( p->x() );
        Q_ASSERT( p->reservedSpace().height() > 0 ); // no such case yet

        //kDebug(11111) << "setMaximalSpace" << ms << cardMap::self()->cardHeight() << cardMap::self()->cardWidth();
        if ( ms.width() < cardWidth - 0.2 )
            return;
        if ( ms.height() < cardHeight - 0.2 )
            return;

        p->setMaximumSpace( ms );
    }

    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        Pile *p = *it;
        if ( !p->isVisible() || ( p->reservedSpace().width() == 10 && p->reservedSpace().height() == 10 ) )
            continue;

        QRectF myRect( p->pos(), p->maximumSpace() );
        if ( p->reservedSpace().width() < 0 )
        {
            myRect.moveRight( p->x() + cardWidth );
            myRect.moveLeft( p->x() - p->maximumSpace().width() );
        }

        //kDebug(11111) << p->objectName() << " " << p->spread() << " " << myRect;

        for ( PileList::ConstIterator it2 = piles.constBegin(); it2 != piles.constEnd(); ++it2 )
        {
            if ( *it2 == p || !( *it2 )->isVisible() )
                continue;

            QRectF pileRect( (*it2)->pos(), (*it2)->maximumSpace() );
            if ( (*it2)->reservedSpace().width() < 0 )
                pileRect.moveRight( (*it2)->x() );
            if ( (*it2)->reservedSpace().height() < 0 )
                pileRect.moveBottom( (*it2)->y() );


            //kDebug(11111) << "compa" << p->objectName() << myRect << ( *it2 )->objectName() << pileRect << myRect.intersects( pileRect );
            // if the growing pile intersects with another pile, we need to solve the conflict
            if ( myRect.intersects( pileRect ) )
            {
                //kDebug(11111) << "compa" << p->objectName() << myRect << ( *it2 )->objectName() << pileRect << myRect.intersects( pileRect );
                QSizeF pms = ( *it2 )->maximumSpace();

                if ( p->reservedSpace().width() != 10 )
                {
                    if ( ( *it2 )->reservedSpace().height() != 10 )
                    {
                        Q_ASSERT( ( *it2 )->reservedSpace().height() > 0 );
                        // if it's growing too, we win
                        pms.setHeight( qMin( myRect.top() - ( *it2 )->y() - 1, pms.height() ) );
                        //kDebug(11111) << "1. reduced height of" << ( *it2 )->objectName();
                    } else // if it's fixed height, we loose
                        if ( p->reservedSpace().width() < 0 ) {
                            // this isn't made for two piles one from left and one from right both growing
                            Q_ASSERT( ( *it2 )->reservedSpace().width() == 10 );
                            myRect.setLeft( ( *it2 )->x() + cardWidth + 1);
                            //kDebug(11111) << "2. reduced width of" << p->objectName();
                        } else {
                            myRect.setRight( ( *it2 )->x() - 1 );
                            //kDebug(11111) << "3. reduced width of" << p->objectName();
                        }
                }

                if ( p->reservedSpace().height() != 10 )
                {
                    if ( ( *it2 )->reservedSpace().width() != 10 )
                    {
                        Q_ASSERT( ( *it2 )->reservedSpace().height() > 0 );
                        // if it's growing too, we win
                        pms.setWidth( qMin( myRect.right() - ( *it2 )->x() - 1, pms.width() ) );
                        //kDebug(11111) << "4. reduced width of" << ( *it2 )->objectName();
                    } else // if it's fixed height, we loose
                        if ( p->reservedSpace().height() < 0 ) {
                            // this isn't made for two piles one from left and one from right both growing
                            Q_ASSERT( ( *it2 )->reservedSpace().height() == 10 );
                            Q_ASSERT( false ); // TODO
                            myRect.setLeft( ( *it2 )->x() + cardWidth + 1);
                            //kDebug(11111) << "5. reduced height of" << p->objectName();
                        } else if ( ( *it2 )->y() >= 1  ) {
                            myRect.setBottom( ( *it2 )->y() - 1 );
                            //kDebug(11111) << "6. reduced height of" << p->objectName() << (*it2)->y() - 1 << myRect;
                        }
                }

                kDebug(11111) << pms  << cardWidth  << cardHeight;
                Q_ASSERT( pms.width() >= cardWidth - 0.1 );
                Q_ASSERT( pms.height() >= cardHeight - 0.1 );
                ( *it2 )->setMaximumSpace( pms );
            }
        }
        kDebug(11111) << myRect << cardHeight;
        Q_ASSERT( myRect.width() >= cardWidth - 0.1 );
        Q_ASSERT( myRect.height() >= cardHeight - 0.1 );

        p->setMaximumSpace( myRect.size() );
    }

    if ( d->wonItem->isVisible() )
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
                kDebug(11111) << "Unknown item type";
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

#include "dealer.moc"
