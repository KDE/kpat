#include "dealer.h"
#include <qobjcoll.h>
#include <kstaticdeleter.h>
#include <kdebug.h>
#include <assert.h>
#include "pile.h"
#include <kmainwindow.h>
#include <qtl.h>
#include <kapp.h>
#include <kmessagebox.h>

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
    : QCanvasView( 0, _parent, _name )
{
    setGameNumber(kapp->random());
    myCanvas.setAdvancePeriod(30);
    myCanvas.setBackgroundColor( darkGreen );
    setCanvas(&myCanvas);
    myCanvas.setDoubleBuffering(true);
    undoList.setAutoDelete(true);
}

Dealer::~Dealer()
{
}

void Dealer::contentsMouseMoveEvent(QMouseEvent* e)
{
    if (movingCards.isEmpty())
        return;

    moved = true;

//    kdDebug() << "Dealer::mouseMoveEvent " << endl;
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
                kdDebug() << "unknown object " << *it << " " << (*it)->rtti() << endl;
            }
        }
    }

    // TODO some caching of the results
    unmarkAll();

    for (PileList::Iterator it = sources.begin(); it != sources.end(); ++it)
    {
        bool b = (*it)->legalAdd(movingCards);
        kdDebug() << "hit " << (*it)->index() << " " << b << endl;
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

    QCanvasItemList list = canvas()->collisions(e->pos());

    kdDebug() << "mouse pressed " << list.count() << " " << canvas()->allItems().count() << endl;
    moved = false;

    if (!list.count())
        return;

    if (list.first()->rtti() == Card::RTTI) {
        Card *c = dynamic_cast<Card*>(list.first());
        assert(c);
        movingCards = c->source()->cardPressed(c);
        kdDebug() << "hit " << c->name() << endl;
        moving_start = e->pos();
        return;
    }

    if (list.first()->rtti() == Pile::RTTI) {
        Pile *c = dynamic_cast<Pile*>(list.first());
        assert(c);
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
    kdDebug() << "Dealer::mouseReleasevent " << moved << endl;
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
            cardClicked(c);
            takeState();
            return;
        }
        if ((*it)->rtti() == Pile::RTTI) {
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
                kdDebug() << "unknown object " << *it << " " << (*it)->rtti() << endl;
            }
        }
    }

    for (HitList::Iterator it = sources.begin(); it != sources.end(); )
    {
        kdDebug() << "hit " << (*it).source->index() << endl;
        if (!(*it).source->legalAdd(movingCards))
            it = sources.remove(it);
        else
            ++it;
    }
    kdDebug() << "count " << sources.count() << endl;

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
    unmarkAll();
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
    cardDblClicked(c);
    takeState();
}

void Dealer::resetSize(const QSize &size) {
    maxsize = size;
    canvas()->resize(size.width(), size.height());
}

void Dealer::cardClicked(Card *c) {
    kdDebug() << "clicked " << c->name() << endl;
}

void Dealer::pileClicked(Pile *c) {
    kdDebug() << "clicked " << c->index() << endl;
}

void Dealer::cardDblClicked(Card *c) {
    kdDebug() << "dbl clicked " << c->name() << endl;
}

void Dealer::startNew()
{
    QCanvasItemList list = canvas()->allItems();
    for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
        (*it)->setAnimated(false);

    undoList.clear();
    restart();
    takeState();
}

void Dealer::enlargeCanvas(QCanvasRectangle *c)
{
    if (!c->visible())
        return;

    bool changed = false;

    if (c->x() + c->width() + 10 > maxsize.width()) {
        maxsize.setWidth(c->x() + c->width() + 10);
        changed = true;
    }
    if (c->y() + c->height() + 10 > maxsize.height()) {
        maxsize.setHeight(c->y() + c->height() + 10);
        changed = true;
    }

    if (changed)
        c->canvas()->resize(maxsize.width(), maxsize.height());
}

void Dealer::viewportResizeEvent ( QResizeEvent *e )
{
    QSize size = canvas()->size();
    QSize msize = e->size();
    kdDebug() << "resize " << msize.width() << " - " << msize.height() << endl;

    bool changed = false;
    if (size.width() > maxsize.width() + 1) {
        size.setWidth(maxsize.width());
        changed = true;
        assert(false);
    }
    if (size.height() > maxsize.height() + 1) {
        size.setHeight(maxsize.height());
        changed = true;
        assert(false);
    }
    if (size.width() < msize.width() - 1) {
        size.setWidth(msize.width());
        changed = true;
    }
    if (size.height() < msize.height() - 1) {
        size.setHeight(msize.height());
        changed = true;
    }

    kdDebug() << "resize2 " << msize.width() << " - " << msize.height() << endl;

    if (changed)
        canvas()->resize(size.width(), size.height());
}

class CardState {
public:
    Card *it;
    Pile *source;
    double x;
    double y;
    double z;
    bool faceup;
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
                i == rhs.i);
    }
};

typedef class QValueList<CardState> CardStateList;

CardStateList *Dealer::getState()
{
    QCanvasItemList list = canvas()->allItems();
    CardStateList *n = new CardStateList;

    for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->rtti() == Pile::RTTI) {
            Pile *p = dynamic_cast<Pile*>(*it);
            assert(p);
        }
    }

    for (QCanvasItemList::ConstIterator it = list.begin(); it != list.end(); ++it)
    {
       if ((*it)->rtti() == Card::RTTI) {
           Card *c = dynamic_cast<Card*>(*it);
           assert(c);
           CardState s;
           s.it = c;
           s.source = c->source();
           s.i = c->source()->indexOf(c);
           s.x = c->realX();
           s.y = c->realY();
           s.z = c->z();
           s.faceup = c->realFace();
           n->append(s);
       }
    }
    qHeapSort(*n);

    return n;
}

void Dealer::setState(CardStateList *n)
{
    QCanvasItemList list = canvas()->allItems();

    for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->rtti() == Pile::RTTI) {
            Pile *p = dynamic_cast<Pile*>(*it);
            assert(p);
            p->clear();
        }
    }

    for (CardStateList::ConstIterator it = n->begin(); it != n->end(); ++it)
    {
        Card *c = (*it).it;
        CardState s = *it;
        s.source->add(c, s.i);
        c->setX(s.x);
        c->setY(s.y);
        c->setZ(s.z);
        c->turn(s.faceup);
    }

    delete n;
    canvas()->update();
}

void Dealer::takeState()
{
    CardStateList *n = getState();

    if (!undoList.count()) {
        undoList.append(getState());
    } else {
        CardStateList *old = undoList.last();

        if (*old == *n) {
            kdDebug() << "nothing changed\n";
            delete n;
            n = 0;
        } else {
            undoList.append(n);
        }
    }

    if (n && isGameWon()) {
        won();
        return;
    }
    emit undoPossible(undoList.count() > 1);
}

void Dealer::undo()
{
    unmarkAll();
    if (undoList.count() > 1) {
        undoList.removeLast(); // the current state
        setState(undoList.take(undoList.count() - 1));
        takeState(); // copying it again
    }
}

long Dealer::gameNumber() const
{
    return gamenumber;
}

void Dealer::setGameNumber(long gmn)
{
    gamenumber = ((gmn + 31998) % 31999) + 1;
}

void Dealer::won()
{
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
	    c->animatedMove( x, y, 0, 20);
       }
    }
    canvas()->update();
    emit gameWon();
}

#include "dealer.moc"
