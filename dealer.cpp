#include "dealer.h"
#include <qobjectlist.h>
#include <kstaticdeleter.h>
#include <qstyle.h>
#include <qpainter.h>
#include <kdebug.h>
#include <assert.h>
#include "pile.h"
#include "kmainwindow.h"
#include <qtl.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <qtimer.h>
#include <kaction.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <cardmaps.h>
#include <speeds.h>

DealerInfoList *DealerInfoList::_self = 0;
static KStaticDeleter<DealerInfoList> dl;

DealerInfoList *DealerInfoList::self()
{
    if (!_self)
        _self = dl.setObject(new DealerInfoList());
    return _self;
}

void DealerInfoList::add(DealerInfo *dealer)
{
    list.append(dealer);
}

Dealer::Dealer( KMainWindow* _parent , const char* _name )
    : QCanvasView( 0, _parent, _name ), towait(0), myActions(0),
ademo(0), ahint(0), aredeal(0),
takeTargets(false), _won(false), _waiting(0), stop_demo_next(false)
{
    setResizePolicy(QScrollView::Manual);
    setVScrollBarMode(AlwaysOff);
    setHScrollBarMode(AlwaysOff);
    setGameNumber(kapp->random());
    myCanvas.setAdvancePeriod(30);
    // myCanvas.setBackgroundColor( darkGreen );
    setCanvas(&myCanvas);
    myCanvas.setDoubleBuffering(true);
    undoList.setAutoDelete(true);
    demotimer = new QTimer(this);
    connect(demotimer, SIGNAL(timeout()), SLOT(demo()));
}

void Dealer::setBackgroundPixmap(const QPixmap &background, const QColor &midcolor)
{
    _midcolor = midcolor;
    canvas()->setBackgroundPixmap(background);
    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
        (*it)->resetCache();
}

void Dealer::setupActions() {

    QPtrList<KAction> actionlist;

    kdDebug(11111) << "setupActions " << actions() << endl;

    if (actions() & Dealer::Hint) {

        ahint = new KAction( i18n("&Hint"), QString::fromLatin1("wizard"), Key_H, this,
                             SLOT(hint()),
                             parent()->actionCollection(), "game_hint");
        actionlist.append(ahint);
    } else
        ahint = 0;

    if (actions() & Dealer::Demo) {
        ademo = new KToggleAction( i18n("&Demo"), QString::fromLatin1("1rightarrow"), CTRL+Key_D, this,
                                   SLOT(toggleDemo()),
                                   parent()->actionCollection(), "game_demo");
        actionlist.append(ademo);
    } else
        ademo = 0;

    if (actions() & Dealer::Redeal) {
        aredeal = new KAction (i18n("&Redeal"), QString::fromLatin1("queue"), 0, this,
                               SLOT(redeal()),
                               parent()->actionCollection(), "game_redeal");
        actionlist.append(aredeal);
    } else
        aredeal = 0;

    parent()->guiFactory()->plugActionList( parent(), QString::fromLatin1("game_actions"), actionlist);
}

Dealer::~Dealer()
{
    clearHints();
    parent()->guiFactory()->unplugActionList( parent(), QString::fromLatin1("game_actions"));

    while (!piles.isEmpty())
        delete piles.first(); // removes itself
}

KMainWindow *Dealer::parent() const
{
    return dynamic_cast<KMainWindow*>(QCanvasView::parent());
}

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
//        kdDebug(11111) << "trying " << store->top()->name() << endl;

        CardList cards = store->cards();
        while (cards.count() && !cards.first()->realFace()) cards.remove(cards.begin());

        CardList::Iterator iti = cards.begin();
        while (iti != cards.end())
        {
            if (store->legalRemove(*iti)) {
//                kdDebug(11111) << "could remove " << (*iti)->name() << endl;
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
            cards.remove(iti);
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
    QCanvasItemList list = canvas()->collisions(movingCards.first()->rect());

    for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->rtti() == Card::RTTI) {
            Card *c = dynamic_cast<Card*>(*it);
            assert(c);
            if (!c->isFaceUp())
                continue;
            if (c->source() == movingCards.first()->source())
                continue;
            if (sources.findIndex(c->source()) != -1)
                continue;
            sources.append(c->source());
        } else {
            if ((*it)->rtti() == Pile::RTTI) {
                Pile *p = static_cast<Pile*>(*it);
                if (p->isEmpty() && !sources.contains(p))
                    sources.append(p);
            } else {
                kdDebug(11111) << "unknown object " << *it << " " << (*it)->rtti() << endl;
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
    for (QCanvasItemList::Iterator it = marked.begin(); it != marked.end(); ++it)
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

    QCanvasItemList list = canvas()->collisions(e->pos());

    kdDebug(11111) << "mouse pressed " << list.count() << " " << canvas()->allItems().count() << endl;
    moved = false;

    if (!list.count())
        return;

    if (e->button() == LeftButton) {
        if (list.first()->rtti() == Card::RTTI) {
            Card *c = dynamic_cast<Card*>(list.first());
            assert(c);
            movingCards = c->source()->cardPressed(c);
            moving_start = e->pos();
            return;
        }
    }

    if (e->button() == RightButton) {
        if (list.first()->rtti() == Card::RTTI) {
            Card *preview = dynamic_cast<Card*>(list.first());
            assert(preview);
            preview->getUp();
        }
    }

    movingCards.clear();
}

class Hit {
public:
    Pile *source;
    QRect intersect;
    bool top;
};
typedef QValueList<Hit> HitList;

void Dealer::contentsMouseReleaseEvent( QMouseEvent *e)
{
    if (!moved) {
        if (!movingCards.isEmpty()) {
            movingCards.first()->source()->moveCardsBack(movingCards);
            movingCards.clear();
        }
        QCanvasItemList list = canvas()->collisions(e->pos());
        if (list.isEmpty())
            return;
        QCanvasItemList::Iterator it = list.begin();
        if ((*it)->rtti() == Card::RTTI) {
            Card *c = dynamic_cast<Card*>(*it);
            assert(c);
            if (!c->animated()) {
                cardClicked(c);
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

    QCanvasItemList list = canvas()->collisions(movingCards.first()->rect());
    HitList sources;

    for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
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
                kdDebug(11111) << "unknown object " << *it << " " << (*it)->rtti() << endl;
            }
        }
    }

    for (HitList::Iterator it = sources.begin(); it != sources.end(); )
    {
        if (!(*it).source->legalAdd(movingCards))
            it = sources.remove(it);
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
    QCanvasItemList list = canvas()->collisions(e->pos());
    if (list.isEmpty())
        return;
    QCanvasItemList::Iterator it = list.begin();
    if ((*it)->rtti() != Card::RTTI)
        return;
    Card *c = dynamic_cast<Card*>(*it);
    assert(c);
    if (!c->animated()) {
        cardDblClicked(c);
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
        QCanvasView::resizeEvent(e);
}

void Dealer::cardClicked(Card *c) {
    kdDebug(11111) << "card clicked " << c->name() << endl;
    c->source()->cardClicked(c);
}

void Dealer::pileClicked(Pile *c) {
    c->cardClicked(0);
}

void Dealer::cardDblClicked(Card *c)
{
    c->source()->cardDblClicked(c);

    kdDebug(11111) << "card dbl clicked " << c->name() << endl;

    if (c->animated())
        return;

    if (c == c->source()->top() && c->realFace()) {
        Pile *tgt = findTarget(c);
        if (tgt) {
            CardList empty;
            empty.append(c);
            c->source()->moveCards(empty, tgt);
            canvas()->update();
        }
    }
}

void Dealer::startNew()
{
    minsize = QSize(0,0);
    _won = false;
    _waiting = 0;
    kdDebug(11111) << "startNew stopDemo\n";
    stopDemo();
    kdDebug(11111) << "startNew unmarkAll\n";
    unmarkAll();
    kdDebug(11111) << "startNew setAnimated(false)\n";
    QCanvasItemList list = canvas()->allItems();
    for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); ++it) {
        if ((*it)->rtti() == Card::RTTI)
            static_cast<Card*>(*it)->disconnect();

        (*it)->setAnimated(true);
        (*it)->setAnimated(false);
    }

    undoList.clear();
    emit undoPossible(false);
    kdDebug(11111) << "startNew restart\n";
    restart();
    takeState();
    Card *towait = 0;
    for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); ++it) {
        if ((*it)->rtti() == Card::RTTI) {
            towait = static_cast<Card*>(*it);
            if (towait->animated())
                break;
        }
    }
    kdDebug(11111) << "startNew takeState\n";
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

void Dealer::enlargeCanvas(QCanvasRectangle *c)
{
    if (!c->isVisible() || c->animated())
        return;

    bool changed = false;

    if (c->x() + c->width() + 10 > minsize.width()) {
        minsize.setWidth(c->x() + c->width() + 10);
        changed = true;
    }
    if (c->y() + c->height() + 10 > minsize.height()) {
        minsize.setHeight(c->y() + c->height() + 10);
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
    int i;
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
                y == rhs.y && z == rhs.z && faceup == rhs.faceup &&
                i == rhs.i && tookdown == rhs.tookdown);
    }

};

typedef class QValueList<CardState> CardStateList;

bool operator==( const State & st1, const State & st2) {
    return st1.cards == st2.cards && st1.gameData == st2.gameData;
}

QDataStream& operator<<( QDataStream& s, const CardState& l ) {
    s << Q_INT8(l.it->value());
    s << Q_INT8(l.it->suit());
    s << Q_INT8(l.source->index());
    s << Q_INT16(l.x) << Q_INT16(l.y) << Q_INT16(l.z) << Q_INT8(l.faceup) << Q_INT8(l.tookdown) << Q_INT8(l.i);
    return s;
}

void Dealer::loadCardState( QDataStream& s, CardState& l, CardList &toload) {
    Q_INT8 v, suit;
    s >> v >> suit;
    l.it = 0;

    for (CardList::Iterator it = toload.begin(); it != toload.end(); ++it)
    {
        if ((*it)->value() == v && (*it)->suit() == suit) {
            l.it = *it;
            toload.remove(it);
            break;
        }
    }
    assert(l.it);
    Q_INT8 index;
    s >> index;
    l.source = 0;
    for (PileList::Iterator it = piles.begin(); it != piles.end(); ++it)
        if ((*it)->index() == index)
            l.source = *it;
    assert(l.source);
    Q_INT8 tookdown, faceup, i;
    Q_INT16 x, y, z;
    s >> x >> y >> z >> faceup >> tookdown >> i;
    l.tookdown = tookdown;
    l.faceup = faceup;
    l.x = x;
    l.y = y;
    l.z = z;
    l.i = i;
}

State *Dealer::getState()
{
    QCanvasItemList list = canvas()->allItems();
    State * st = new State;

    for (QCanvasItemList::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
       if ((*it)->rtti() == Card::RTTI) {
           Card *c = dynamic_cast<Card*>(*it);
           assert(c);
           CardState s;
           s.it = c;
           s.source = c->source();
           if (!s.source) {
               kdDebug(11111) << c->name() << " has no parent\n";
               assert(false);
           }
           s.i = c->source()->indexOf(c);
           s.x = c->realX();
           s.y = c->realY();
           s.z = c->realZ();
           s.faceup = c->realFace();
           s.tookdown = c->takenDown();
           st->cards.append(s);
       }
    }
    qHeapSort(st->cards);

    // Game specific information
    QDataStream stream( st->gameData, IO_WriteOnly );
    getGameState( stream );

    return st;
}

void Dealer::setState(State *st)
{
    CardStateList * n = &st->cards;
    QCanvasItemList list = canvas()->allItems();

    for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
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
        s.source->add(c, s.i);
        c->setVisible(s.source->isVisible());
        c->setAnimated(false);
        c->setX(s.x);
        c->setY(s.y);
        c->setZ(s.z);
        c->setTakenDown(s.tookdown || (target && !s.source->target()));
        c->turn(s.faceup);
    }

    // restore game-specific information
    QDataStream stream( st->gameData, IO_ReadOnly );
    setGameState(stream);

    delete st;
    canvas()->update();
}

void Dealer::takeState()
{
    kdDebug(11111) << "takeState\n";

    State *n = getState();

    if (!undoList.count()) {
        undoList.append(n);
    } else {
        State *old = undoList.last();

        if (*old == *n) {
            delete n;
            n = 0;
        } else {
            undoList.append(n);
        }
    }

    if (n) {
        if (isGameWon()) {
            won();
            return;
        } else if (!demoActive() && !waiting()) {
            QTimer::singleShot(TIME_BETWEEN_MOVES, this, SLOT(startAutoDrop()));
        }
    }
    emit undoPossible(undoList.count() > 1 && !waiting());
}

void Dealer::saveGame(QDataStream &s) {
    s << 0; // file format
    s << _id; // dealer number
    s << Q_ULONG (gameNumber());
    s << undoList.count();
    QPtrListIterator<State> it(undoList);

    for (; it.current(); ++it)
    {
        State *n = it.current();
        s << n->gameData;
        s << n->cards;
    }
}

void Dealer::openGame(QDataStream &s)
{
    unmarkAll();
    Q_ULONG gn;
    s >> gn;
    setGameNumber(gn);
    uint count;
    s >> count;
    undoList.clear();

    for (; count != 0; count--)
    {
        CardList cards;

        QCanvasItemList list = canvas()->allItems();
        for (QCanvasItemList::ConstIterator it = list.begin(); it != list.end(); ++it)
            if ((*it)->rtti() == Card::RTTI)
                cards.append(static_cast<Card*>(*it));

        State *n = new State();
        s >> n->gameData;
        n->cards.clear();
        Q_UINT32 c;
        s >> c;
        for( Q_UINT32 i = 0; i < c; ++i )
        {
            CardState t;
            loadCardState(s, t, cards);
            n->cards.append( t );
        }
        assert(cards.isEmpty());
        undoList.append(n);
    }

    if (undoList.count() > 1) {
        setState(undoList.take(undoList.count() - 1));
        takeState(); // copying it again
        emit undoPossible(undoList.count() > 1);
    }
    takeState();
}

void Dealer::undo()
{
    unmarkAll();
    stopDemo();
    if (undoList.count() > 1) {
        undoList.removeLast(); // the current state
        setState(undoList.take(undoList.count() - 1));
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
    kdDebug(11111) << "setWaiting " << w << " " << _waiting << endl;
}

bool Dealer::startAutoDrop()
{
    if (movingCards.count())
        return false;

    kdDebug(11111) << "startAutoDrop\n";

    unmarkAll();
    clearHints();
    getHints();
    for (HintList::ConstIterator it = hints.begin(); it != hints.end(); ++it) {
        MoveHint *mh = *it;
        if (mh->pile()->target() && mh->dropIfTarget() && !mh->card()->takenDown()) {
            setWaiting(true);
            Card *t = mh->card();
            CardList cards = mh->card()->source()->cards();
            while (cards.count() && cards.first() != t) cards.remove(cards.begin());
            t->setAnimated(false);
            t->turn(true);
            int x = int(t->x());
            int y = int(t->y());
            t->source()->moveCards(cards, mh->pile());
            t->move(x, y);
            kdDebug(11111) << "autodrop " << t->name() << endl;
            t->animatedMove(t->source()->x(), t->source()->y(), t->z(), STEPS_AUTODROP);
            connect(t, SIGNAL(stoped(Card*)), SLOT(waitForAutoDrop(Card*)));
            return true;
        }
    }
    clearHints();
    return false;
}

void Dealer::waitForAutoDrop(Card * c) {
    kdDebug(11111) << "waitForAutoDrop " << c->name() << endl;
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
    gamenumber = ((gmn + 31998) % 31999) + 1;
}

void Dealer::addPile(Pile *p)
{
    piles.append(p);
}

void Dealer::removePile(Pile *p)
{
    piles.remove(p);
}

void Dealer::stopDemo()
{
    kdDebug(11111) << "stopDemo " << waiting() << " " << stop_demo_next << endl;
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

void Dealer::won()
{
    if (_won)
        return;
    _won = true;
    QCanvasItemList list = canvas()->allItems();
    for (QCanvasItemList::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->rtti() == Card::RTTI)
        {
            Card *c = dynamic_cast<Card*>(*it);
            assert(c);
            c->turn(true);
            QRect p(0, 0, c->width(), c->height());
            QRect can(0, 0, canvas()->width(), canvas()->height());
            int x, y;

            do {
                // disperse the cards everywhere
                x = 3*canvas()->width()/2 - kapp->random() % (canvas()->width() * 2);
                y = 3*canvas()->height()/2 - (kapp->random() % (canvas()->height() * 2));
                p.moveTopLeft(QPoint(x, y));
            } while (can.intersects(p));
	    c->animatedMove( x, y, 0, STEPS_WON);
       }
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
            t->animatedMove(x2, y2, t->z(), STEPS_DEMO);
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
    demotimer->start(250, true);
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

bool Dealer::checkRemove( int, const Pile *, const Card *) const {
    return true;
}

bool Dealer::checkAdd( int, const Pile *, const CardList&) const {
    return true;
}

void Dealer::drawPile(KPixmap &pixmap, Pile *pile, bool selected)
{
    QPixmap bg = myCanvas.backgroundPixmap();
    QRect bounding(pile->x(), pile->y(), cardMap::CARDX, cardMap::CARDY);

    pixmap.resize(bounding.width(), bounding.height());
    pixmap.fill(Qt::white);

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
            bitBlt(&pixmap, dx, dy, &bg,
                   sx, sy, w, h, Qt::CopyROP, true);
        }
    }

    float s = -0.1;
    float n = -0.3;

    int mid = (midColor().red() + midColor().green() + midColor().blue()) / 3;

    // if it's too dark - light instead of dark
    if (mid < 120) {
        s *= -1;
        n = 0.4;
    }

    KPixmapEffect::intensity(pixmap, selected ? s : n);

    QPainter painter(&pixmap);

    QColorGroup colgrp( Qt::black, Qt::white, midColor().light(),
                        midColor().dark(), midColor(), Qt::black,
                        Qt::white );
#if QT_VERSION < 300
    kapp->style().drawPanel( &painter, 0, 0, cardMap::CARDX, cardMap::CARDY, colgrp, true );
#endif
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
    kdDebug(11111) << "setAnchorname " << name << endl;
    ac = name;
}

QString Dealer::anchorName() const { return ac; }

MoveHint::MoveHint(Card * c, Pile *_to, bool d)
{
    _card = c; to = _to; _dropiftarget = d;
}

#include "dealer.moc"
