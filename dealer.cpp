#include "dealer.h"
#include <qobjcoll.h>
#include <kstaticdeleter.h>
#include <kdebug.h>
#include <assert.h>
#include "pile.h"
#include <kmainwindow.h>

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
    myCanvas.resize(100, 200);
    myCanvas.setAdvancePeriod(30);
    myCanvas.setBackgroundColor( darkGreen );
    setCanvas(&myCanvas);
    myCanvas.setDoubleBuffering(true);
}

Dealer::~Dealer()
{
}

QSize Dealer::sizeHint() const
{
    // just a dummy size
    return QSize( 900, 500 );
}

#include "dealer.moc"

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
        kdDebug() << "hit pile " << c->index() << endl;
    }

    movingCards.clear();
}

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
            return;
        }
        if ((*it)->rtti() == Pile::RTTI) {
            Pile *c = dynamic_cast<Pile*>(*it);
            assert(c);
            pileClicked(c);
            return;
        }
    }

    if (!movingCards.count())
        return;
    Card *c = dynamic_cast<Card*>(movingCards.first());
    assert(c);

    unmarkAll();

    QCanvasItemList list = canvas()->collisions(movingCards.first()->rect());
    PileList sources;

    for (QCanvasItemList::Iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it)->rtti() == Card::RTTI) {
            Card *c = dynamic_cast<Card*>(*it);
            assert(c);
            if (!c->isFaceUp())
                continue;
            if (c->source() == movingCards.first()->source())
                continue;
            if (sources.contains(c->source()))
                continue;
            sources.append(c->source());
        } else {
            if ((*it)->rtti() == Pile::RTTI) {
                Pile *p = static_cast<Pile*>(*it);
                if (!sources.contains(p))
                    sources.append(p);
            } else {
                kdDebug() << "unknown object " << *it << " " << (*it)->rtti() << endl;
            }
        }
    }

    for (PileList::Iterator it = sources.begin(); it != sources.end(); )
    {
        kdDebug() << "hit " << (*it)->index() << endl;
        if (!(*it)->legalAdd(movingCards))
            it = sources.remove(it);
        else
            ++it;
    }
    kdDebug() << "count " << sources.count() << endl;

    if (sources.count() != 1) {
        c->source()->moveCardsBack(movingCards);
    } else {
        c->source()->moveCards(movingCards, sources.first());
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
