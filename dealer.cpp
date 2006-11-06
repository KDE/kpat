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
#include <QGraphicsView>
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
#include "patsolve/patsolve.h"
#include <math.h>

// ================================================================
//                         class MoveHint

MoveHint::MoveHint(Card *card, Pile *to, bool d)
{
    m_card         = card;
    m_to           = to;
    m_dropiftarget = d;
}


// ================================================================
//                          class CardState


class CardState {
public:
    Card *it;
    Pile *source;
    qreal x;
    qreal y;
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
        return (it == rhs.it && source == rhs.source && x == rhs.x &&
                y == rhs.y && z == rhs.z && faceup == rhs.faceup
                && source_index == rhs.source_index && tookdown == rhs.tookdown);
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

void DealerScene::takeState()
{
    // kDebug(11111) << "takeState " << waiting() << endl;

    QList<QGraphicsItem *> list = items();
    for (QList<QGraphicsItem *>::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId ) {
            Card *c = static_cast<Card*>(*it);
            if (c->animated()) {
                QTimer::singleShot(100, this, SLOT(takeState()));
                return;
            }
        }
    }

    State *n = getState();

    if (!undoList.count()) {
        emit updateMoves();
        undoList.append(n);
    } else {
        State *old = undoList.last();

        if (*old == *n) {
            delete n;
            n = 0;
        } else {
            emit updateMoves();
            undoList.append(n);
        }
    }

    if (n) {
        if (isGameWon()) {
            won();
            return;
        }
        else if ( isGameLost() && !toldAboutLostGame) {
            emit hintPossible( false );
            emit demoPossible( false );
            emit redealPossible( false );
            QTimer::singleShot(400, this, SIGNAL(gameLost()));
            toldAboutLostGame = true;
            return;
        }
    }
    if (!demoActive() && !waiting())
        QTimer::singleShot(TIME_BETWEEN_MOVES, this, SLOT(startAutoDrop()));

    emit undoPossible(undoList.count() > 1);
}

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
                kDebug(11111) << "pile index " << p->index() << " taken twice\n";
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
                card.setAttribute("name", (*it)->name());
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
        for (QValueList<CardState>::ConstIterator it2 = n->cards.begin();
             it2 != n->cards.end(); ++it2)
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
    // kDebug(11111) << doc.toString() << endl;
}

void DealerScene::openGame(QDomDocument &doc)
{
    unmarkAll();
    QDomElement dealer = doc.documentElement();

    setGameNumber(dealer.attribute("number").toULong());

    QDomNodeList piles = dealer.elementsByTagName("pile");

    QList<QGraphicsItem *> list = items();

    CardList cards;
    for (QList<QGraphicsItem *>::ConstIterator it = list.begin(); it != list.end(); ++it)
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId )
            cards.append(static_cast<Card*>(*it));

    Deck::deck()->collectAndShuffle();

    for (QList<QGraphicsItem *>::Iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::PileTypeId )
        {
            Pile *p = dynamic_cast<Pile*>(*it);
            assert(p);

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
                            if ((*it2)->suit() == s && (*it2)->rank() == v) {
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

    if (undoList.count() > 1) {
        setState(undoList.takeLast());
        takeState(); // copying it again
        emit undoPossible(undoList.count() > 1);
    }

    emit updateMoves();
    takeState();
}

void DealerScene::undo()
{
    unmarkAll();
    stopDemo();
    kDebug() << "::undo " << undoList.count() << endl;
    if (undoList.count() > 1) {
        delete undoList.last();
        undoList.removeLast(); // the current state
        setState(undoList.takeLast());
        emit updateMoves();
        takeState(); // copying it again
        emit undoPossible(undoList.count() > 1);
        if ( toldAboutLostGame ) { // everything's possible again
            hintPossible( true );
            demoPossible( true );
            toldAboutLostGame = false;
        }
    }
}


// ================================================================
//                         class DealerScene


DealerScene::DealerScene():
    _autodrop(true),
    _waiting(0),
    gamenumber( 0 ),
    demo_active( false ),
    stop_demo_next(false),
    _won(false),
    _gameRecorded(false),
    wonItem( 0 ),
    gothelp(false),
    myActions(0),
    m_solver(0)
{
    demotimer = new QTimer(this);
    connect(demotimer, SIGNAL(timeout()), SLOT(demo()));
    m_autoDropFactor = 1;
    connect( this, SIGNAL( gameWon( bool ) ), SLOT( slotShowGame(bool) ) );
}

DealerScene::~DealerScene()
{
    kDebug() << "~DealerScene " << endl;
    if (!_won)
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
    delete m_solver;
}


// ----------------------------------------------------------------


void DealerScene::hint()
{
    unmarkAll();
    clearHints();
    getHints();
    for (HintList::ConstIterator it = hints.begin(); it != hints.end(); ++it)
        mark((*it)->card());
    clearHints();
}

void DealerScene::getHints()
{
    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        if ((*it)->target())
            continue;

        Pile *store = *it;
        if (store->isEmpty())
            continue;
//        kDebug(11111) << "trying " << store->top()->name() << endl;

        CardList cards = store->cards();
        while (cards.count() && !cards.first()->realFace()) cards.erase(cards.begin());

        CardList::Iterator iti = cards.begin();
        while (iti != cards.end())
        {
            if (store->legalRemove(*iti)) {
//                kDebug(11111) << "could remove " << (*iti)->name() << endl;
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
                        newHint(new MoveHint(*iti, dest));
                    else {
                        store->hideCards(cards);
                        // if it could be here as well, then it's no use
                        if ((store->isEmpty() && !dest->isEmpty()) || !store->legalAdd(cards))
                            newHint(new MoveHint(*iti, dest));
                        else {
                            if (old_prefer && !checkPrefering( store->checkIndex(),
                                                               store, cards ))
                            { // if checkPrefers says so, we add it nonetheless
                                newHint(new MoveHint(*iti, dest));
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
    for (HintList::Iterator it = hints.begin(); it != hints.end(); ++it)
        delete *it;
    hints.clear();
}

void DealerScene::newHint(MoveHint *mh)
{
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

    PileList sources;
    QList<QGraphicsItem *> list = collidingItems( movingCards.first() );

    // kDebug() << "movingCards " << movingCards.first()->sceneBoundingRect() << endl;
    for (QList<QGraphicsItem *>::Iterator it = list.begin(); it != list.end(); ++it)
    {
        //kDebug() << "it " << ( *it )->type() << " " << ( *it )->sceneBoundingRect() << endl;
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId) {
            Card *c = dynamic_cast<Card*>(*it);
            assert(c);
            if (!c->isFaceUp())
                continue;
            if (c->source() == movingCards.first()->source())
                continue;
            if (sources.indexOf(c->source()) != -1)
                continue;
            sources.append(c->source());
        } else {
            if ((*it)->type() == QGraphicsItem::UserType + DealerScene::PileTypeId ) {
                Pile *p = static_cast<Pile*>(*it);
                if (p->isEmpty() && !sources.contains(p))
                    sources.append(p);
            } else {
                kDebug(11111) << "unknown object " << *it << " " << (*it)->type() << endl;
            }
        }
    }

    // TODO some caching of the results
    unmarkAll();

    for (PileList::Iterator it = sources.begin(); it != sources.end(); ++it)
    {
        bool b = (*it)->legalAdd(movingCards);
        // kDebug() << "legalAdd " << b << " " << ( *it )->x() << endl;
        if (b) {
            if ((*it)->isEmpty()) {
                (*it)->setHighlighted(true);
                marked.append(*it);
            } else {
                mark((*it)->top());
            }
        }
    }

    moving_start = e->scenePos();
}

void DealerScene::mark(Card *c)
{
//    kDebug() << "mark " << c->name() << endl;
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
    unmarkAll();
    stopDemo();
    if (waiting())
        return;

    QList<QGraphicsItem *> list = items( e->scenePos() );

    kDebug(11111) << "mouse pressed " << list.count() << " " << items().count() << " " << e->scenePos().x() << " " << e->scenePos().y() << endl;
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

void DealerScene::startNew()
{
    if (!_won)
        countLoss();
    _won = false;
    _waiting = 0;
    _gameRecorded=false;
    delete wonItem;
    wonItem = 0;
    gothelp = false;
    qDeleteAll(undoList);
    undoList.clear();
    toldAboutLostGame = false;
    emit updateMoves();

    stopDemo();
    kDebug(11111) << "startNew unmarkAll\n";
    unmarkAll();
    kDebug(11111) << "startNew setAnimated(false)\n";
    QList<QGraphicsItem *> list = items();
    for (QList<QGraphicsItem *>::Iterator it = list.begin(); it != list.end(); ++it) {
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId )
        {
            Card *c = static_cast<Card*>(*it);
            c->disconnect();
            c->stopAnimation();
        }
    }
    emit undoPossible(false);
    kDebug(11111) << "startNew restart\n";
    restart();
    takeState();
    update();
}

void DealerScene::slotShowGame(bool gothelp)
{
    kDebug() << "slotShowGame " << waiting() << endl;

    int wx = qRound( width() * 0.7 );
    int wy = qRound( height() * 0.6 );

    QImage img = QImage( wx, wy, QImage::Format_ARGB32 );
    img.fill( qRgba( 0, 0, 255, 0 ) );
    QPainter p( &img );

    QSvgRenderer renderer( KStandardDirs::locate( "data", "kpat/won.svg" ) );
    renderer.render( &p, "frame" );

    QString text = i18n( "Congratulation! You have won." );
    if ( gothelp )
        text = i18n( "Congratulation! We have won." );
    QFont font;
    font.setPointSize( 36 );

    int twidth = QFontMetrics( font ).width( text );
    int fontsize = 36;
    while ( twidth > wx * 0.9 )
    {
        fontsize--;
        font.setPointSize( fontsize );
        twidth = QFontMetrics( font ).width ( text );
    }

    p.setFont( font );
    p.drawText( int( ( wx - twidth ) / 2 ),
                int( ( wy - QFontMetrics( font ).descent() ) / 2 ),
                text );
    p.end();

    QGraphicsPixmapItem *item = new QGraphicsPixmapItem( 0, this );
    item->setPixmap( QPixmap::fromImage( img ) );
    item->setZValue( 2000 );

    wonItem = item;

    wonItem->setPos( QPointF( ( width() - wonItem->sceneBoundingRect().width() ) / 2,
                              ( height() - wonItem->sceneBoundingRect().height() ) / 2 ) );

    emit demoPossible( false );
    emit hintPossible( false );
    // emit undoPossible(false); // technically it's possible but cheating :)
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
            }
            return;
        }
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::PileTypeId ) {
            Pile *c = dynamic_cast<Pile*>(*it);
            assert(c);
            pileClicked(c);
            takeState();
            return;
        }
    }

    if (!movingCards.count())
        return;
    Card *c = static_cast<Card*>(movingCards.first());
    assert(c);

    unmarkAll();

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
                kDebug(11111) << "unknown object " << *it << " " << (*it)->type() << endl;
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

    if (sources.isEmpty()) {
        c->source()->moveCardsBack(movingCards);
    } else {
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
        countGame();
        c->source()->moveCards(movingCards, (*best).source);
        takeState();
    }
    movingCards.clear();
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
    kDebug() << "mouseDoubleClickEvent " << list << endl;
    if (list.isEmpty())
        return;
    QList<QGraphicsItem *>::Iterator it = list.begin();
    if ((*it)->type() != QGraphicsItem::UserType + DealerScene::CardTypeId )
        return;
    Card *c = dynamic_cast<Card*>(*it);
    assert(c);
    c->stopAnimation();
    kDebug() << "card " << c->name() << endl;
    if ( cardDblClicked(c) ) {
        countGame();
    }
    takeState();
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

    for (QList<QGraphicsItem *>::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
       if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId ) {
           Card *c = dynamic_cast<Card*>(*it);
           assert(c);
           CardState s;
           s.it = c;
           s.source = c->source();
           if (!s.source) {
               kDebug(11111) << c->name() << " has no parent\n";
               assert(false);
           }
           s.source_index = c->source()->indexOf(c);
           s.x = c->realX();
           s.y = c->realY();
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

    for (CardStateList::ConstIterator it = n->begin(); it != n->end(); ++it)
    {
        Card *c = (*it).it;
        c->stopAnimation();
        CardState s = *it;
        // kDebug() << "c " << c->name() << " " << s.source->objectName() << " " << s.faceup << endl;
        bool target = c->takenDown(); // abused
        s.source->add(c, s.source_index);
        c->setVisible(s.source->isVisible());
        c->setPos(QPointF( s.x, s.y) );
        c->setZValue(int(s.z));
        c->setTakenDown(s.tookdown || (target && !s.source->target()));
        c->turn(s.faceup);
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
    for (PileList::ConstIterator it = piles.begin(); it != piles.end(); ++it)
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
    if (w)
        _waiting++;
    else if ( _waiting > 0 )
        _waiting--;
//    kDebug(11111) << "setWaiting " << w << " " << _waiting << endl;
}

void DealerScene::setAutoDropEnabled(bool a)
{
    _autodrop = a;
    QTimer::singleShot(TIME_BETWEEN_MOVES, this, SLOT(startAutoDrop()));
}

bool DealerScene::startAutoDrop()
{
    if ( solver() )
        solver()->showCurrentMoves();

    if (!autoDrop())
        return false;

    if (!movingCards.isEmpty()) {
        QTimer::singleShot(qRound( TIME_BETWEEN_MOVES * m_autoDropFactor ), this, SLOT(startAutoDrop()));
        return true;
    }


    QList<QGraphicsItem *> list = items();

    //kDebug( 11111 ) << "startAutoDrop\n";
    for (QList<QGraphicsItem *>::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId ) {
            Card *c = static_cast<Card*>(*it);
            if (c->animated()) {
                QTimer::singleShot(qRound( TIME_BETWEEN_MOVES * m_autoDropFactor ), this, SLOT(startAutoDrop()));
                // kDebug() << "animation still going on\n";
                return true;
            }
        }
    }

    //kDebug(11111) << "startAutoDrop2\n";

    unmarkAll();
    clearHints();
    getHints();
    for (HintList::ConstIterator it = hints.begin(); it != hints.end(); ++it) {
        MoveHint *mh = *it;
        if (mh->pile()->target() && mh->dropIfTarget() && !mh->card()->takenDown()) {
            setWaiting(true);
            Card *t = mh->card();
            CardList cards = mh->card()->source()->cards();
            while (cards.count() && cards.first() != t)
                cards.erase(cards.begin());
            t->stopAnimation();
            t->turn(true);
            qreal x = t->x();
            qreal y = t->y();
            t->source()->moveCards(cards, mh->pile());
            t->stopAnimation();
            t->setPos(QPointF( x, y) );
            //kDebug(11111) << "autodrop " << t->name() << endl;
            t->moveTo(t->source()->x(), t->source()->y(), t->zValue(), qRound( DURATION_AUTODROP * m_autoDropFactor ) );
            connect(t, SIGNAL(stoped(Card*)), SLOT(waitForAutoDrop(Card*)));
            m_autoDropFactor *= 0.8;
            return true;
        }
    }
    clearHints();
    m_autoDropFactor = 1;
    return false;
}

void DealerScene::waitForAutoDrop(Card * c) {
    //kDebug(11111) << "waitForAutoDrop " << c->name() << endl;
    setWaiting(false);
    c->disconnect();
    c->stopAnimation(); // should be a no-op
    takeState();
}

void DealerScene::waitForWonAnim(Card *c)
{
    //kDebug() << "waitForWonAnim " << c->name() << " " << waiting() << endl;
    if ( !waiting() )
        return;

    c->setVisible( false ); // hide it
    c->disconnect( this, SLOT( waitForWonAnim( Card* ) ) );

    setWaiting(false);

    if ( !waiting() )
    {
        stopDemo();
        emit gameWon(gothelp);
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
    for (PileList::ConstIterator it = piles.begin(); it != piles.end(); ++it)
    {
        ( *it )->rescale();
    }
}

void DealerScene::setGameNumber(long gmn)
{
    // Deal in the range of 1 to INT_MAX.
    gamenumber = ((gmn < 1) ? 1 : ((gmn > 0x7FFFFFFF) ? 0x7FFFFFFF : gmn));
    qDeleteAll(undoList);
    undoList.clear();
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
    if (waiting()) {
        _waiting = 0;
        stop_demo_next = true;
        return;
    } else stop_demo_next = false;

    QList<QGraphicsItem *> list = items();
    for (QList<QGraphicsItem *>::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId ) {
            Card *c = static_cast<Card*>(*it);
            c->stopAnimation();
        }
    }

    demotimer->stop();
    demo_active = false;
    emit demoActive( false );
}

bool DealerScene::demoActive() const
{
    return demo_active;
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
    kDebug() << "won " << waiting() << endl;
    if (_won)
        return;
    _won = true;

    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        ( *it )->relayoutCards(); // stop the timer
    }

    // update score, 'win' in demo mode also counts (keep it that way?)
    KConfigGroup kc(KGlobal::config(), scores_group);
    unsigned int n = kc.readEntry(QString("won%1").arg(_id),0) + 1;
    kc.writeEntry(QString("won%1").arg(_id),n);
    n = kc.readEntry(QString("winstreak%1").arg(_id),0) + 1;
    kc.writeEntry(QString("winstreak%1").arg(_id),n);
    unsigned int m = kc.readEntry(QString("maxwinstreak%1").arg(_id),0);
    if (n>m)
        kc.writeEntry(QString("maxwinstreak%1").arg(_id),n);
    kc.writeEntry(QString("loosestreak%1").arg(_id),0);

    // sort cards by increasing z
    QList<QGraphicsItem *> list = items();
    QList<CardPtr> cards;
    for (QList<QGraphicsItem *>::ConstIterator it=list.begin(); it!=list.end(); ++it)
        if ((*it)->type() == QGraphicsItem::UserType + DealerScene::CardTypeId ) {
            CardPtr p;
            p.ptr = dynamic_cast<Card*>(*it);
            assert(p.ptr);
            cards.push_back(p);
        }
    qSort(cards);

    // disperse the cards everywhere
    QRectF can(0, 0, width(), height());
    QListIterator<CardPtr> it(cards);
    while (it.hasNext()) {
        CardPtr card = it.next();
        card.ptr->turn(true);
        QRectF p = card.ptr->sceneBoundingRect();
        p.moveTo( 0, 0 );
        qreal x, y;
        do {
            x = 3*width()/2 - KRandom::random() % int(width() * 2);
            y = 3*height()/2 - (KRandom::random() % int(height() * 2));
            p.moveTopLeft(QPointF(x, y));
        } while (can.intersects(p));

	card.ptr->moveTo( x, y, 0, DURATION_WON);
        connect(card.ptr, SIGNAL(stoped(Card*)), SLOT(waitForWonAnim(Card*)));
        setWaiting(true);
    }
}

MoveHint *DealerScene::chooseHint()
{
    if (hints.isEmpty())
        return 0;

    for (HintList::ConstIterator it = hints.begin(); it != hints.end(); ++it)
    {
        if ((*it)->pile()->target() && (*it)->dropIfTarget())
            return *it;
    }

    return hints[randseq.getLong(hints.count())];
}

void DealerScene::demo()
{
    kDebug() << "demo " << waiting() << endl;
    if ( waiting() )
        return;

    if (stop_demo_next) {
        stopDemo();
        return;
    }
    stop_demo_next = false;
    demo_active = true;
    gothelp = true;
    unmarkAll();
    clearHints();
    getHints();
    demotimer->stop();

    if ( solver() )
        kDebug() << "return " << solver()->patsolve() << endl;
    return;

    MoveHint *mh = chooseHint();
    if (mh) {
        // assert(mh->card()->source()->legalRemove(mh->card()));

        CardList empty;
        CardList cards = mh->card()->source()->cards();
        bool after = false;
        for (CardList::Iterator it = cards.begin(); it != cards.end(); ++it) {
            if (*it == mh->card())
                after = true;
            if (after)
                empty.append(*it);
        }

        assert(!empty.isEmpty());

        qreal *oldcoords = new qreal[2*empty.count()];
        int i = 0;

        for (CardList::Iterator it = empty.begin(); it != empty.end(); ++it) {
            Card *t = *it;
            t->stopAnimation();
            t->turn(true);
            oldcoords[i++] = t->realX();
            oldcoords[i++] = t->realY();
        }

        assert(mh->card()->source() != mh->pile());
        // assert(mh->pile()->legalAdd(empty));

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
        Card *t = demoNewCards();
        if (t) {
            newDemoMove(t);
        } else if (isGameWon()) {
            emit gameWon(true);
            return;
        } else
            stopDemo();
    }

    emit demoActive( true );
    takeState();
}

Card *DealerScene::demoNewCards()
{
    return 0;
}

void DealerScene::newDemoMove(Card *m)
{
    kDebug() << "newDemoMove " << m->name() << endl;
    setWaiting( true );
    connect(m, SIGNAL(stoped(Card*)), SLOT(waitForDemo(Card*)));
}

void DealerScene::waitForDemo(Card *t)
{
    kDebug() << "waitForDemo " << t->name() << endl;
    t->disconnect(this, SLOT( waitForDemo( Card* ) ) );
    setWaiting( false );
    if ( !waiting() )
    {
        demotimer->setSingleShot( true );
        demotimer->start(250);
    }
}

bool DealerScene::isGameWon() const
{
    for (PileList::ConstIterator it = piles.begin(); it != piles.end(); ++it)
    {
        if (!(*it)->target() && !(*it)->isEmpty())
            return false;
    }
    return true;
}

bool DealerScene::isGameLost() const
{
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
    if ( !_gameRecorded ) {
        kDebug(11111) << "counting game as played." << endl;
        KConfigGroup kc(KGlobal::config(), scores_group);
        unsigned int Total = kc.readEntry(QString("total%1").arg(_id),0);
        ++Total;
        kc.writeEntry(QString("total%1").arg(_id),Total);
        _gameRecorded = true;
    }
}

void DealerScene::countLoss()
{
    if ( _gameRecorded ) {
        // update score
        KConfigGroup kc(KGlobal::config(), scores_group);
        unsigned int n = kc.readEntry(QString("loosestreak%1").arg(_id),0) + 1;
        kc.writeEntry(QString("loosestreak%1").arg(_id),n);
        unsigned int m = kc.readEntry(QString("maxloosestreak%1").arg(_id),0);
        if (n>m)
            kc.writeEntry(QString("maxloosestreak%1").arg(_id),n);
        kc.writeEntry(QString("winstreak%1").arg(_id),0);
    }
}

QPointF DealerScene::maxPilePos() const
{
    QPointF maxp( 0, 0 );
    for (PileList::ConstIterator it = piles.begin(); it != piles.end(); ++it)
    {
        // kDebug() << ( *it )->objectName() << " " <<  ( *it )->pilePos() << " " << ( *it )->reservedSpace() << endl;
        // we call fabs here, because reserved space of -50 means for a pile at -1x-1 means it wants at least
        // 49 all together. Even though the real logic should be more complex, it's enough for our current needs
        maxp = QPointF( qMax( fabs( ( *it )->pilePos().x() + ( *it )->reservedSpace().width() ), maxp.x() ),
                        qMax( fabs( ( *it )->pilePos().y() + ( *it )->reservedSpace().height() ), maxp.y() ) );
    }
    return maxp;
}

void DealerScene::relayoutPiles()
{
    if ( !views().isEmpty() )
        setSceneSize( views().first()->viewport()->size() );
}

void DealerScene::setSceneSize( const QSize &s )
{
    //kDebug() << "setSceneSize " << kBacktrace() << endl;
    setSceneRect( QRectF( 0, 0, s.width(), s.height() ) );

    QPointF defaultSceneRect = maxPilePos();

    kDebug() << "setSceneSize " << defaultSceneRect << " " << s << endl;
    Q_ASSERT( cardMap::self()->wantedCardWidth() > 0 );
    Q_ASSERT( cardMap::self()->wantedCardHeight() > 0 );

    qreal scaleX = s.width() / cardMap::self()->wantedCardWidth() / ( defaultSceneRect.x() + 1 );
    qreal scaleY = s.height() / cardMap::self()->wantedCardHeight() / ( defaultSceneRect.y() + 1 );
    kDebug() << "scaleY " << scaleY << " " << scaleX << endl;
    qreal n_scaleFactor = qMin(scaleX, scaleY);

    kDebug() << "scale " << n_scaleFactor << endl;
    cardMap::self()->setWantedCardWidth( int( n_scaleFactor * 10 * cardMap::self()->wantedCardWidth() ) );

    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        Pile *p = *it;
        if ( p->pilePos().x() < 0 || p->pilePos().y() < 0 )
            p->rescale();
        QSizeF ms( cardMap::self()->wantedCardWidth(),
                   cardMap::self()->wantedCardHeight() );
        //kDebug() << p->objectName() <<  " " << p->reservedSpace() << " " << p->pos() << endl;
        if ( p->reservedSpace().width() > 10 )
            ms.setWidth( s.width() - p->x() );
        if ( p->reservedSpace().height() > 10 )
            ms.setHeight( s.height() - p->y() );
        if ( p->reservedSpace().width() < 0 )
            ms.setWidth( p->x() );
        Q_ASSERT( p->reservedSpace().height() > 0 ); // no such case yet

        // kDebug() << "setMaximalSpace " << ms << " " << cardMap::self()->wantedCardHeight() << " " << cardMap::self()->wantedCardWidth() << endl;
        Q_ASSERT( ms.width() >= cardMap::self()->wantedCardWidth() - 0.2 );
        Q_ASSERT( ms.height() >= cardMap::self()->wantedCardHeight() - 0.2 );

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

        // kDebug() << p->objectName() << " " << p->spread() << " " << myRect << endl;

        for ( PileList::ConstIterator it2 = piles.begin(); it2 != piles.end(); ++it2 )
        {
            if ( *it2 == p || !( *it2 )->isVisible() )
                continue;

            QRectF pileRect( (*it2)->pos(), (*it2)->maximalSpace() );
            if ( (*it2)->reservedSpace().width() < 0 )
                pileRect.moveRight( (*it2)->x() );
            if ( (*it2)->reservedSpace().height() < 0 )
                pileRect.moveBottom( (*it2)->y() );


            // kDebug() << "compa " << p->objectName() << " " << myRect << " " << ( *it2 )->objectName() << " " << pileRect << " " << myRect.intersects( pileRect ) << endl;
            // if the growing pile intersects with another pile, we need to solve the conflict
            if ( myRect.intersects( pileRect ) )
            {
                // kDebug() << "compa " << p->objectName() << " " << myRect << " " << ( *it2 )->objectName() << " " << pileRect << " " << myRect.intersects( pileRect ) << endl;
                QSizeF pms = ( *it2 )->maximalSpace();

                if ( p->reservedSpace().width() != 10 )
                {
                    if ( ( *it2 )->reservedSpace().height() != 10 )
                    {
                        Q_ASSERT( ( *it2 )->reservedSpace().height() > 0 );
                        // if it's growing too, we win
                        pms.setHeight( qMin( myRect.top() - ( *it2 )->y() - 1, pms.height() ) );
                        // kDebug() << "1. reduced height of " << ( *it2 )->objectName() << endl;
                    } else // if it's fixed height, we loose
                        if ( p->reservedSpace().width() < 0 ) {
                            // this isn't made for two piles one from left and one from right both growing
                            Q_ASSERT( ( *it2 )->reservedSpace().width() == 10 );
                            myRect.setLeft( ( *it2 )->x() + cardMap::self()->wantedCardWidth() + 1);
                            // kDebug() << "2. reduced width of " << p->objectName() << endl;
                        } else {
                            myRect.setRight( ( *it2 )->x() - 1 );
                            // kDebug() << "3. reduced width of " << p->objectName() << endl;
                        }
                }

                if ( p->reservedSpace().height() != 10 )
                {
                    if ( ( *it2 )->reservedSpace().width() != 10 )
                    {
                        Q_ASSERT( ( *it2 )->reservedSpace().height() > 0 );
                        // if it's growing too, we win
                        pms.setWidth( qMin( myRect.right() - ( *it2 )->x() - 1, pms.width() ) );
                        kDebug() << "4. reduced width of " << ( *it2 )->objectName() << endl;
                    } else // if it's fixed height, we loose
                        if ( p->reservedSpace().height() < 0 ) {
                            // this isn't made for two piles one from left and one from right both growing
                            Q_ASSERT( ( *it2 )->reservedSpace().height() == 10 );
                            Q_ASSERT( false ); // TODO
                            myRect.setLeft( ( *it2 )->x() + cardMap::self()->wantedCardWidth() + 1);
                            kDebug() << "5. reduced height of " << p->objectName() << endl;
                        } else {
                            myRect.setBottom( ( *it2 )->y() - 1 );
                            // kDebug() << "6. reduced height of " << p->objectName() << endl;
                        }
                }

                Q_ASSERT( pms.width() >= cardMap::self()->wantedCardWidth() - 0.1 );
                Q_ASSERT( pms.height() >= cardMap::self()->wantedCardHeight() - 0.1 );
                ( *it2 )->setMaximalSpace( pms );
            }
        }
        //kDebug() << myRect << " " << cardMap::self()->wantedCardHeight() << endl;
        Q_ASSERT( myRect.width() >= cardMap::self()->wantedCardWidth() - 0.1 );
        Q_ASSERT( myRect.height() >= cardMap::self()->wantedCardHeight() - 0.1 );

        p->setMaximalSpace( myRect.size() );
    }

    if ( wonItem )
        wonItem->setPos( QPointF( ( width() - wonItem->sceneBoundingRect().width() ) / 2,
                                  ( height() - wonItem->sceneBoundingRect().height() ) / 2 ) );

    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        ( *it )->relayoutCards();
    }
}

#include "dealer.moc"
