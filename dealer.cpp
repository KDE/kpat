#include "dealer.h"
#include <kstaticdeleter.h>
#include <kdebug.h>
#include "deck.h"
#include <assert.h>
#include "kmainwindow.h"
#include <kapplication.h>
#include <kpixmapeffect.h>
#include <kxmlguifactory.h>
#include <qtimer.h>
//Added by qt3to4:
#include <QWheelEvent>
#include <QPixmap>
#include <Q3PtrList>
#include <QList>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QPainter>
#include <krandom.h>
#include <kaction.h>
#include <klocale.h>
#include "cardmaps.h"
#include "speeds.h"
#include <kconfig.h>
#include <kglobal.h>
#include "version.h"


// ================================================================
//                         class MoveHint


MoveHint::MoveHint(Card *card, Pile *to, bool d)
{
    m_card         = card;
    m_to           = to;
    m_dropiftarget = d;
}


// ================================================================
//                    class DealerInfoList


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


Dealer *Dealer::s_instance = 0;


Dealer::Dealer( KMainWindow* _parent , const char* _name )
  : Q3CanvasView( 0, _parent, _name ),
    towait(0),
    myActions(0),
    ademo(0),
    ahint(0),
    aredeal(0),
    takeTargets(false),
    _won(false),
    _waiting(0),
    stop_demo_next(false),
    _autodrop(true),
    _gameRecorded(false)
{
    setResizePolicy(Q3ScrollView::Manual);
    setVScrollBarMode(AlwaysOff);
    setHScrollBarMode(AlwaysOff);

    setGameNumber(KRandom::random());
    myCanvas.setAdvancePeriod(30);
    // myCanvas.setBackgroundColor( darkGreen );
    setCanvas(&myCanvas);
    myCanvas.setDoubleBuffering(true);

    undoList.setAutoDelete(true);

    demotimer = new QTimer(this);

    connect(demotimer, SIGNAL(timeout()), SLOT(demo()));

    assert(!s_instance);
    s_instance = this;
}


const Dealer *Dealer::instance()
{
    return s_instance;
}


void Dealer::setBackgroundPixmap(const QPixmap &background, const QColor &midcolor)
{
    _midcolor = midcolor;
    canvas()->setBackgroundPixmap(background);
    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it) {
        (*it)->resetCache();
        (*it)->initSizes();
    }
}

void Dealer::setupActions() {

    QList<KAction*> actionlist;

    kDebug(11111) << "setupActions " << actions() << endl;

    if (actions() & Dealer::Hint) {

        ahint = new KAction( i18n("&Hint"),
                             parent()->actionCollection(), "game_hint");
        ahint->setCustomShortcut( Qt::Key_H );
        ahint->setIcon( KIcon( "wizard" ) );
        connect( ahint, SIGNAL( triggered( bool ) ), SLOT(hint()) );
        actionlist.append(ahint);
    } else
        ahint = 0;

    if (actions() & Dealer::Demo) {
        ademo = new KToggleAction( i18n("&Demo"), parent()->actionCollection(), "game_demo");
        ademo->setIcon( KIcon( "1rightarrow") );
        ademo->setCustomShortcut( Qt::CTRL+Qt::Key_D );
        connect( ademo, SIGNAL( triggered( bool ) ), SLOT( toggleDemo() ) );
        actionlist.append(ademo);
    } else
        ademo = 0;

    if (actions() & Dealer::Redeal) {
        aredeal = new KAction (i18n("&Redeal"), parent()->actionCollection(), "game_redeal");
        aredeal->setIcon( KIcon( "queue") );
        connect( aredeal, SIGNAL( triggered( bool ) ), SLOT( redeal() ) );
        actionlist.append(aredeal);
    } else
        aredeal = 0;

    parent()->guiFactory()->plugActionList( parent(), QString::fromLatin1("game_actions"), actionlist);
}

Dealer::~Dealer()
{
	 if (!_won)
		 countLoss();
    clearHints();
    parent()->guiFactory()->unplugActionList( parent(), QString::fromLatin1("game_actions"));

    while (!piles.isEmpty())
        delete piles.first(); // removes itself

    if (s_instance == this)
        s_instance = 0;
}

KMainWindow *Dealer::parent() const
{
    return dynamic_cast<KMainWindow*>(Q3CanvasView::parent());
}


// ----------------------------------------------------------------


void Dealer::hint()
{
    unmarkAll();
    clearHints();
    getHints();
    for (HintList::ConstIterator it = hints.begin(); it != hints.end(); ++it)
        mark((*it)->card());
    clearHints();
    canvas()->update();
}


void Dealer::getHints()
{
    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
    {
        if (!takeTargetForHints() && (*it)->target())
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
                    if (!takeTargetForHints() && dest->target())
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

bool Dealer::checkPrefering( int /*checkIndex*/, const Pile *, const CardList& ) const
{
    return false;
}

void Dealer::clearHints()
{
    for (HintList::Iterator it = hints.begin(); it != hints.end(); ++it)
        delete *it;
    hints.clear();
}

void Dealer::newHint(MoveHint *mh)
{
    hints.append(mh);
}

bool Dealer::isMoving(Card *c) const
{
    return movingCards.indexOf(c) != -1;
}

void Dealer::contentsMouseMoveEvent(QMouseEvent* e)
{
    if (movingCards.isEmpty())
        return;

    moved = true;

    for (CardList::Iterator it = movingCards.begin(); it != movingCards.end(); ++it)
    {
        (*it)->moveBy(e->pos().x() - moving_start.x(),
                      e->pos().y() - moving_start.y());
    }

    PileList sources;
    Q3CanvasItemList list = canvas()->collisions(movingCards.first()->rect());

    for (Q3CanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->rtti() == Card::RTTI) {
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
            if ((*it)->rtti() == Pile::RTTI) {
                Pile *p = static_cast<Pile*>(*it);
                if (p->isEmpty() && !sources.contains(p))
                    sources.append(p);
            } else {
                kDebug(11111) << "unknown object " << *it << " " << (*it)->rtti() << endl;
            }
        }
    }

    // TODO some caching of the results
    unmarkAll();

    for (PileList::Iterator it = sources.begin(); it != sources.end(); ++it)
    {
        bool b = (*it)->legalAdd(movingCards);
        if (b) {
            if ((*it)->isEmpty()) {
                (*it)->setSelected(true);
                marked.append(*it);
            } else {
                mark((*it)->top());
            }
        }
    }

    moving_start = e->pos();
    canvas()->update();
}

void Dealer::mark(Card *c)
{
    c->setSelected(true);
    if (!marked.contains(c))
        marked.append(c);
}

void Dealer::unmarkAll()
{
    for (Q3CanvasItemList::Iterator it = marked.begin(); it != marked.end(); ++it)
    {
        (*it)->setSelected(false);
    }
    marked.clear();
}

void Dealer::contentsMousePressEvent(QMouseEvent* e)
{
    unmarkAll();
    stopDemo();
    if (waiting())
        return;

    Q3CanvasItemList list = canvas()->collisions(e->pos());

    kDebug(11111) << "mouse pressed " << list.count() << " " << canvas()->allItems().count() << endl;
    moved = false;

    if (!list.count())
        return;

    if (e->button() == Qt::LeftButton) {
        if (list.first()->rtti() == Card::RTTI) {
            Card *c = dynamic_cast<Card*>(list.first());
            assert(c);
            CardList mycards = c->source()->cardPressed(c);
            for (CardList::Iterator it = mycards.begin(); it != mycards.end(); ++it)
                (*it)->setAnimated(false);
            movingCards = mycards;
            moving_start = e->pos();
        }
        return;
    }

    if (e->button() == Qt::RightButton) {
        if (list.first()->rtti() == Card::RTTI) {
            Card *preview = dynamic_cast<Card*>(list.first());
            assert(preview);
            if (!preview->animated() && !isMoving(preview))
                preview->getUp();
        }
        return;
    }

    // if it's nothing else, we move the cards back
    contentsMouseReleaseEvent(e);

}

class Hit {
public:
    Pile *source;
    QRect intersect;
    bool top;
};
typedef QList<Hit> HitList;

void Dealer::contentsMouseReleaseEvent( QMouseEvent *e)
{
    if (!moved) {
        if (!movingCards.isEmpty()) {
            movingCards.first()->source()->moveCardsBack(movingCards);
            movingCards.clear();
        }
        Q3CanvasItemList list = canvas()->collisions(e->pos());
        if (list.isEmpty())
            return;
        Q3CanvasItemList::Iterator it = list.begin();
        if ((*it)->rtti() == Card::RTTI) {
            Card *c = dynamic_cast<Card*>(*it);
            assert(c);
            if (!c->animated()) {
                if ( cardClicked(c) ) {
                    countGame();
                }
                takeState();
                canvas()->update();
            }
            return;
        }
        if ((*it)->rtti() == Pile::RTTI) {
            Pile *c = dynamic_cast<Pile*>(*it);
            assert(c);
            pileClicked(c);
            takeState();
            canvas()->update();
            return;
        }
    }

    if (!movingCards.count())
        return;
    Card *c = static_cast<Card*>(movingCards.first());
    assert(c);

    unmarkAll();

    Q3CanvasItemList list = canvas()->collisions(movingCards.first()->rect());
    HitList sources;

    for (Q3CanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->rtti() == Card::RTTI) {
            Card *c = dynamic_cast<Card*>(*it);
            assert(c);
            if (!c->isFaceUp())
                continue;
            if (c->source() == movingCards.first()->source())
                continue;
            Hit t;
            t.source = c->source();
            t.intersect = c->rect().intersect(movingCards.first()->rect());
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
            if ((*it)->rtti() == Pile::RTTI) {
                Pile *p = static_cast<Pile*>(*it);
                if (p->isEmpty())
                {
                    Hit t;
                    t.source = p;
                    t.intersect = p->rect().intersect(movingCards.first()->rect());
                    t.top = true;
                    sources.append(t);
                }
            } else {
                kDebug(11111) << "unknown object " << *it << " " << (*it)->rtti() << endl;
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
    canvas()->update();
}

void Dealer::contentsMouseDoubleClickEvent( QMouseEvent*e )
{
    stopDemo();
    unmarkAll();
    if (waiting())
        return;

    if (!movingCards.isEmpty()) {
        movingCards.first()->source()->moveCardsBack(movingCards);
        movingCards.clear();
    }
    Q3CanvasItemList list = canvas()->collisions(e->pos());
    if (list.isEmpty())
        return;
    Q3CanvasItemList::Iterator it = list.begin();
    if ((*it)->rtti() != Card::RTTI)
        return;
    Card *c = dynamic_cast<Card*>(*it);
    assert(c);
    if (!c->animated()) {
        if ( cardDblClicked(c) ) {
            countGame();
        }
        takeState();
    }
}

QSize Dealer::minimumCardSize() const
{
    return minsize;
}

void Dealer::resizeEvent(QResizeEvent *e)
{
    int x = width();
    int y = height();
    int hs = horizontalScrollBar()->sizeHint().height();
    int vs = verticalScrollBar()->sizeHint().width();

    int mx = minsize.width();
    int my = minsize.height();

    int dx = x;
    int dy = y;
    bool showh = false;
    bool showv = false;

    if (mx > x) {
        dx = mx;
        if (my + vs < y)
            dy -= vs;
        else {
            showv = true;
        }
        showh = true;
    } else if (my > y) {
        dy = my;
        if (mx + hs < x)
            dx -= hs;
        else
            showh = true;
        showv = true;
    }
    canvas()->resize(dx, dy);
    resizeContents(dx, dy);
    setVScrollBarMode(showv ? AlwaysOn : AlwaysOff);
    setHScrollBarMode(showh ? AlwaysOn : AlwaysOff);

    if (!e)
        updateScrollBars();
    else
        Q3CanvasView::resizeEvent(e);
}

bool Dealer::cardClicked(Card *c) {
    return c->source()->cardClicked(c);
}

void Dealer::pileClicked(Pile *c) {
    c->cardClicked(0);
}

bool Dealer::cardDblClicked(Card *c)
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
            canvas()->update();
            return true;
        }
    }
    return false;
}

void Dealer::startNew()
{
    if (!_won)
		 countLoss();
    if ( ahint )
        ahint->setEnabled( true );
    if ( ademo )
        ademo->setEnabled( true );
    if ( aredeal )
        aredeal->setEnabled( true );
    toldAboutLostGame = false;
    minsize = QSize(0,0);
    _won = false;
    _waiting = 0;
    _gameRecorded=false;
    kDebug(11111) << "startNew stopDemo\n";
    stopDemo();
    kDebug(11111) << "startNew unmarkAll\n";
    unmarkAll();
    kDebug(11111) << "startNew setAnimated(false)\n";
    Q3CanvasItemList list = canvas()->allItems();
    for (Q3CanvasItemList::Iterator it = list.begin(); it != list.end(); ++it) {
        if ((*it)->rtti() == Card::RTTI)
            static_cast<Card*>(*it)->disconnect();

        (*it)->setAnimated(true);
        (*it)->setAnimated(false);
    }

    undoList.clear();
    emit undoPossible(false);
    emit updateMoves();
    kDebug(11111) << "startNew restart\n";
    restart();
    takeState();
    Card *towait = 0;
    for (Q3CanvasItemList::Iterator it = list.begin(); it != list.end(); ++it) {
        if ((*it)->rtti() == Card::RTTI) {
            towait = static_cast<Card*>(*it);
            if (towait->animated())
                break;
        }
    }

    kDebug(11111) << "startNew takeState\n";
    if (!towait)
        takeState();
    else
        connect(towait, SIGNAL(stoped(Card*)), SLOT(slotTakeState(Card *)));
    resizeEvent(0);
}

void Dealer::slotTakeState(Card *c) {
    if (c)
        c->disconnect();
    takeState();
}

void Dealer::enlargeCanvas(Q3CanvasRectangle *c)
{
    if (!c->isVisible() || c->animated())
        return;

    bool changed = false;

    if (c->x() + c->width() + 10 > minsize.width()) {
        minsize.setWidth(int(c->x()) + c->width() + 10);
        changed = true;
    }
    if (c->y() + c->height() + 10 > minsize.height()) {
        minsize.setHeight(int(c->y()) + c->height() + 10);
        changed = true;
    }
    if (changed)
        resizeEvent(0);
}

class CardState {
public:
    Card *it;
    Pile *source;
    double x;
    double y;
    double z;
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
        e.setAttribute("source", source->index());
        e.setAttribute("x", x);
        e.setAttribute("y", y);
        e.setAttribute("z", z);
        e.setAttribute("faceup", faceup);
        e.setAttribute("tookdown", tookdown);
        e.setAttribute("source_index", source_index);
    }
};

typedef class QList<CardState> CardStateList;

bool operator==( const State & st1, const State & st2) {
    return st1.cards == st2.cards && st1.gameData == st2.gameData;
}

State *Dealer::getState()
{
    Q3CanvasItemList list = canvas()->allItems();
    State * st = new State;

    for (Q3CanvasItemList::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
       if ((*it)->rtti() == Card::RTTI) {
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

void Dealer::setState(State *st)
{
    CardStateList * n = &st->cards;
    Q3CanvasItemList list = canvas()->allItems();

    for (Q3CanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->rtti() == Pile::RTTI) {
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
        CardState s = *it;
        bool target = c->takenDown(); // abused
        s.source->add(c, s.source_index);
        c->setVisible(s.source->isVisible());
        c->setAnimated(false);
        c->setX(s.x);
        c->setY(s.y);
        c->setZ(int(s.z));
        c->setTakenDown(s.tookdown || (target && !s.source->target()));
        c->turn(s.faceup);
    }

    // restore game-specific information
    setGameState( st->gameData );

    delete st;
    canvas()->update();
}

void Dealer::takeState()
{
    kDebug(11111) << "takeState\n";

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
        else if (isGameLost() && !toldAboutLostGame) {
            if ( ahint )
                ahint->setEnabled( false );
            if ( ademo )
                ademo->setEnabled( false );
            if ( aredeal )
                aredeal->setEnabled( false );
            QTimer::singleShot(400, this, SIGNAL(gameLost()));
            toldAboutLostGame = true;
            return;
        }
    }
    if (!demoActive() && !waiting())
            QTimer::singleShot(TIME_BETWEEN_MOVES, this, SLOT(startAutoDrop()));

    emit undoPossible(undoList.count() > 1 && !waiting());
}

void Dealer::saveGame(QDomDocument &doc) {
    QDomElement dealer = doc.createElement("dealer");
    doc.appendChild(dealer);
    dealer.setAttribute("id", _id);
    dealer.setAttribute("number", QString::number(gameNumber()));
    QString data = getGameState();
    if (!data.isEmpty())
        dealer.setAttribute("data", data);
    dealer.setAttribute("moves", QString::number(getMoves()));

    bool taken[1000];
    memset(taken, 0, sizeof(taken));

    Q3CanvasItemList list = canvas()->allItems();
    for (Q3CanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->rtti() == Pile::RTTI) {
            Pile *p = dynamic_cast<Pile*>(*it);
            assert(p);
            if (taken[p->index()]) {
                kDebug(11111) << "pile index " << p->index() << " taken twice\n";
                return;
            }
            taken[p->index()] = true;

            QDomElement pile = doc.createElement("pile");
            pile.setAttribute("index", p->index());

            CardList cards = p->cards();
            for (CardList::Iterator it = cards.begin();
                 it != cards.end();
                 ++it)
            {
                QDomElement card = doc.createElement("card");
                card.setAttribute("suit", (*it)->suit());
                card.setAttribute("value", (*it)->rank());
                card.setAttribute("faceup", (*it)->isFaceUp());
                card.setAttribute("x", (*it)->realX());
                card.setAttribute("y", (*it)->realY());
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

void Dealer::openGame(QDomDocument &doc)
{
    unmarkAll();
    QDomElement dealer = doc.documentElement();

    setGameNumber(dealer.attribute("number").toULong());
    undoList.clear();

    QDomNodeList piles = dealer.elementsByTagName("pile");

    Q3CanvasItemList list = canvas()->allItems();

    CardList cards;
    for (Q3CanvasItemList::ConstIterator it = list.begin(); it != list.end(); ++it)
        if ((*it)->rtti() == Card::RTTI)
            cards.append(static_cast<Card*>(*it));

    Deck::deck()->collectAndShuffle();

    for (Q3CanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->rtti() == Pile::RTTI)
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
                                if (QString((*it2)->name()) == "Diamonds Eight") {
                                    kDebug(11111) << i << " " << j << endl;
                                }
                                p->add(*it2);
                                (*it2)->setAnimated(false);
                                (*it2)->turn(card.attribute("faceup").toInt());
                                (*it2)->setX(card.attribute("x").toInt());
                                (*it2)->setY(card.attribute("y").toInt());
                                (*it2)->setZ(card.attribute("z").toInt());
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
        setState(undoList.take(undoList.count() - 1));
        takeState(); // copying it again
        emit undoPossible(undoList.count() > 1);
    }

    emit updateMoves();
    takeState();
}

void Dealer::undo()
{
    unmarkAll();
    stopDemo();
    if (undoList.count() > 1) {
        undoList.removeLast(); // the current state
        setState(undoList.take(undoList.count() - 1));
        emit updateMoves();
        takeState(); // copying it again
        emit undoPossible(undoList.count() > 1);
    }
}

Pile *Dealer::findTarget(Card *c)
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

void Dealer::setWaiting(bool w)
{
    if (w)
        _waiting++;
    else
        _waiting--;
    emit undoPossible(!waiting());
    kDebug(11111) << "setWaiting " << w << " " << _waiting << endl;
}

void Dealer::setAutoDropEnabled(bool a)
{
    _autodrop = a;
    QTimer::singleShot(TIME_BETWEEN_MOVES, this, SLOT(startAutoDrop()));
}

bool Dealer::startAutoDrop()
{
    if (!autoDrop())
        return false;

    Q3CanvasItemList list = canvas()->allItems();

    for (Q3CanvasItemList::ConstIterator it = list.begin(); it != list.end(); ++it)
        if ((*it)->animated()) {
            QTimer::singleShot(TIME_BETWEEN_MOVES, this, SLOT(startAutoDrop()));
            return true;
        }

    kDebug(11111) << "startAutoDrop\n";

    unmarkAll();
    clearHints();
    getHints();
    for (HintList::ConstIterator it = hints.begin(); it != hints.end(); ++it) {
        MoveHint *mh = *it;
        if (mh->pile()->target() && mh->dropIfTarget() && !mh->card()->takenDown()) {
            setWaiting(true);
            Card *t = mh->card();
            CardList cards = mh->card()->source()->cards();
            while (cards.count() && cards.first() != t) cards.erase(cards.begin());
            t->setAnimated(false);
            t->turn(true);
            int x = int(t->x());
            int y = int(t->y());
            t->source()->moveCards(cards, mh->pile());
            t->move(x, y);
            kDebug(11111) << "autodrop " << t->name() << endl;
            t->moveTo(int(t->source()->x()), int(t->source()->y()), int(t->z()), STEPS_AUTODROP);
            connect(t, SIGNAL(stoped(Card*)), SLOT(waitForAutoDrop(Card*)));
            return true;
        }
    }
    clearHints();
    return false;
}

void Dealer::waitForAutoDrop(Card * c) {
    kDebug(11111) << "waitForAutoDrop " << c->name() << endl;
    setWaiting(false);
    c->disconnect();
    takeState();
}

long Dealer::gameNumber() const
{
    return gamenumber;
}

void Dealer::setGameNumber(long gmn)
{
    // Deal in the range of 1 to INT_MAX.
    gamenumber = ((gmn < 1) ? 1 : ((gmn > 0x7FFFFFFF) ? 0x7FFFFFFF : gmn));
}

void Dealer::addPile(Pile *p)
{
    piles.append(p);
}

void Dealer::removePile(Pile *p)
{
    piles.removeAll(p);
}

void Dealer::stopDemo()
{
    kDebug(11111) << "stopDemo " << waiting() << " " << stop_demo_next << endl;
    if (waiting()) {
        stop_demo_next = true;
        return;
    } else stop_demo_next = false;

    if (towait == (Card*)-1)
        towait = 0;

    if (towait) {
        towait->disconnect();
        towait = 0;
    }
    demotimer->stop();
    if (ademo)
        ademo->setChecked(false);
}

bool Dealer::demoActive() const
{
    return (towait || demotimer->isActive());
}

void Dealer::toggleDemo()
{
    if (demoActive()) {
        stopDemo();
    } else
        demo();
}

class CardPtr
{
 public:
    Card *ptr;
};

bool operator <(const CardPtr &p1, const CardPtr &p2)
{
    return ( p1.ptr->z() < p2.ptr->z() );
}

void Dealer::won()
{
    if (_won)
        return;
    _won = true;

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
    Q3CanvasItemList list = canvas()->allItems();
    QList<CardPtr> cards;
    for (Q3CanvasItemList::ConstIterator it=list.begin(); it!=list.end(); ++it)
        if ((*it)->rtti() == Card::RTTI) {
            CardPtr p;
            p.ptr = dynamic_cast<Card*>(*it);
            assert(p.ptr);
            cards.push_back(p);
        }
    qSort(cards);

    // disperse the cards everywhere
    QRect can(0, 0, canvas()->width(), canvas()->height());
    QListIterator<CardPtr> it(cards);
    while (it.hasNext()) {
        CardPtr card = it.next();
        card.ptr->turn(true);
        QRect p(0, 0, card.ptr->width(), card.ptr->height());
        int x, y;
        do {
            x = 3*canvas()->width()/2 - KRandom::random() % (canvas()->width() * 2);
            y = 3*canvas()->height()/2 - (KRandom::random() % (canvas()->height() * 2));
            p.moveTopLeft(QPoint(x, y));
        } while (can.intersects(p));

	card.ptr->moveTo( x, y, 0, STEPS_WON);
    }

    bool demo = demoActive();
    stopDemo();
    canvas()->update();
    emit gameWon(demo);
}

MoveHint *Dealer::chooseHint()
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

void Dealer::demo() {
    if (waiting())
        return;

    if (stop_demo_next) {
        stopDemo();
        return;
    }
    stop_demo_next = false;
    unmarkAll();
    towait = (Card*)-1;
    clearHints();
    getHints();
    demotimer->stop();

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

        int *oldcoords = new int[2*empty.count()];
        int i = 0;

        for (CardList::Iterator it = empty.begin(); it != empty.end(); ++it) {
            Card *t = *it;
            Q_ASSERT(!t->animated());
            t->setAnimated(false);
            t->turn(true);
            oldcoords[i++] = int(t->realX());
            oldcoords[i++] = int(t->realY());
        }

        assert(mh->card()->source() != mh->pile());
        // assert(mh->pile()->legalAdd(empty));

        mh->card()->source()->moveCards(empty, mh->pile());

        i = 0;

        for (CardList::Iterator it = empty.begin(); it != empty.end(); ++it) {
            Card *t = *it;
            int x1 = oldcoords[i++];
            int y1 = oldcoords[i++];
            int x2 = int(t->realX());
            int y2 = int(t->realY());
            t->move(x1, y1);
            t->moveTo(x2, y2, int(t->z()), STEPS_DEMO);
        }

        delete [] oldcoords;

        newDemoMove(mh->card());

    } else {
        Card *t = demoNewCards();
        if (t) {
            newDemoMove(t);
        } else if (isGameWon()) {
            canvas()->update();
            emit gameWon(true);
            return;
        } else
            stopDemo();
    }

    takeState();
}

Card *Dealer::demoNewCards()
{
    return 0;
}

void Dealer::newDemoMove(Card *m)
{
    towait = m;
    connect(m, SIGNAL(stoped(Card*)), SLOT(waitForDemo(Card*)));
}

void Dealer::waitForDemo(Card *t)
{
    if (t == (Card*)-1)
        return;
    if (towait != t)
        return;
    t->disconnect();
    towait = 0;
    demotimer->setSingleShot( true );
    demotimer->start(250);
}

bool Dealer::isGameWon() const
{
    for (PileList::ConstIterator it = piles.begin(); it != piles.end(); ++it)
    {
        if (!(*it)->target() && !(*it)->isEmpty())
            return false;
    }
    return true;
}

bool Dealer::isGameLost() const
{
    return false;
}

bool Dealer::checkRemove( int, const Pile *, const Card *) const {
    return true;
}

bool Dealer::checkAdd( int, const Pile *, const CardList&) const {
    return true;
}

void Dealer::drawPile(QPixmap &pixmap, Pile *pile, bool selected)
{
    QPixmap bg = myCanvas.backgroundPixmap();
    QRect bounding(int(pile->x()), int(pile->y()), cardMap::CARDX(), cardMap::CARDY());

    pixmap = QPixmap(bounding.width(), bounding.height());
    pixmap.fill(Qt::white);

    if (!bg.isNull()) {
        QPainter paint(&pixmap);
        for (int x=bounding.x()/bg.width();
             x<(bounding.x()+bounding.width()+bg.width()-1)/bg.width(); x++)
        {
            for (int y=bounding.y()/bg.height();
                 y<(bounding.y()+bounding.height()+bg.height()-1)/bg.height(); y++)
            {
                int sx = 0;
                int sy = 0;
                int dx = x*bg.width()-bounding.x();
                int dy = y*bg.height()-bounding.y();
                int w = bg.width();
                int h = bg.height();
                if (dx < 0) {
                    sx = -dx;
                dx = 0;
                }
                if (dy < 0) {
                    sy = -dy;
                    dy = 0;
                }
                paint.drawPixmap(dx, dy, bg, sx, sy, w, h);
            }
        }
    }


    float s = -0.4;
    float n = -0.3;

    int mid = qMax( qMax(midColor().red(), midColor().green()), midColor().blue());

    // if it's too dark - light instead of dark
    if (mid < 120) {
        s *= -1;
        n = 0.4;
    }

    //QImage img = pixmap.toImage().mirrored( false, true );
    //pixmap = QPixmap::fromImage( img, Qt::AutoColor );
    //kDebug() <<  pixmap.fromImage( img, QPixmap::Auto ) << endl;
    //KPixmapEffect::gradient( pixmap, Qt::white, Qt::red, KPixmapEffect::VerticalGradient );
    KPixmapEffect::intensity(pixmap, selected ? s : n);
    //KPixmapEffect::blend( pixmap, 0.3, Qt::red, KPixmapEffect::VerticalGradient );
}

int Dealer::freeCells() const
{
    int n = 0;
    for (PileList::ConstIterator it = piles.begin(); it != piles.end(); ++it)
        if ((*it)->isEmpty() && !(*it)->target())
            n++;
    return n;
}

void Dealer::setAnchorName(const QString &name)
{
    kDebug(11111) << "setAnchorname " << name << endl;
    ac = name;
}

QString Dealer::anchorName() const { return ac; }

void Dealer::wheelEvent( QWheelEvent *e )
{
    QWheelEvent ce( viewport()->mapFromGlobal( e->globalPos() ),
                    e->delta(), e->buttons(), e->modifiers() );
    viewportWheelEvent(&ce);
    if ( !ce.isAccepted() ) {
	if ( e->orientation() == Qt::Horizontal && hScrollBarMode () == AlwaysOn )
	    QApplication::sendEvent( horizontalScrollBar(), e);
	else  if (e->orientation() == Qt::Vertical && vScrollBarMode () == AlwaysOn )
	    QApplication::sendEvent( verticalScrollBar(), e);
    } else {
	e->accept();
    }
}

void Dealer::countGame()
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

void Dealer::countLoss()
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

#include "dealer.moc"
