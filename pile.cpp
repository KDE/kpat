#include "pile.h"
#include "dealer.h"
#include "card.h"
#include <kdebug.h>
#include <qpainter.h>
#include "cardmaps.h"
#include <qdrawutil.h>
#include <kapp.h>
#include <assert.h>
#include <kpixmap.h>

const int Pile::Default       = 0x0000;
const int Pile::disallow      = 0x0001;
const int Pile::several       = 0x0002; // default: move one card

// Add-flags
const int Pile::addSpread     = 0x0100;

// Remove-flags
const int Pile::autoTurnTop   = 0x0200;
const int Pile::wholeColumn   = 0x0400;

int Pile::SPREAD = 20;
int Pile::DSPREAD = 5;
int Pile::HSPREAD = 9;

Card *Pile::top() const
{
    if (myCards.isEmpty())
        return 0;

    return myCards.last();
}

Pile::Pile( int _index, Dealer* parent)
    : QCanvasRectangle( parent->canvas() ), _dealer(parent), myIndex(_index), _target(false)
{
    dealer()->addPile(this);
    QCanvasRectangle::setVisible(true); // default
    setSize( cardMap::CARDX, cardMap::CARDY );
    _checkIndex = -1;
    addFlags = removeFlags = 0;
    setBrush(Qt::black);
    setPen(QPen(Qt::black));
    setZ(0);
}

Pile::~Pile()
{
    dealer()->removePile(this);

    for (CardList::Iterator it = myCards.begin(); it != myCards.end(); ++it)
    {
        if ((*it)->source() != this) {
            int i = -13;
            if ((*it)->source())
                i = (*it)->source()->index();
            kdDebug() << "pile doesn't match " << index() << " - " << i << endl;
        }
        (*it)->setSource(0);
    }
}

void Pile::resetCache()
{
    cache.resize(0, 0);
    cache_selected.resize(0, 0);
}

void Pile::drawShape ( QPainter & painter )
{
    if (selected()) {
        if (cache.isNull())
            dealer()->drawPile(cache, this, false);
        painter.drawPixmap(int(x()), int(y()), cache);
    } else {
        if (cache_selected.isNull())
            dealer()->drawPile(cache_selected, this, true);
        painter.drawPixmap(int(x()), int(y()), cache_selected);
    }
}

bool Pile::legalAdd( const CardList& _card ) const
{
    if( addFlags & disallow ) {
        return false;
    }

    if (!legalMoves.isEmpty() && !legalMoves.contains(_card.first()->source()->index()))
    {
        return false;
    }

    if( !( addFlags & several ) &&
        _card.count() > 1 )
    {
        return false;
    }

    bool result = dealer()->checkAdd( checkIndex(), this, _card );
    return result;
}

bool Pile::legalRemove(const Card *c) const
{
    if( removeFlags & disallow ) {
        return false;
    }
    if( !( removeFlags & several ) && top() != c)
    {
        return false;
    }

    bool b = dealer()->checkRemove( checkIndex(), this, c);
    return b;
}

void Pile::setVisible(bool vis)
{
    QCanvasRectangle::setVisible(vis);
    dealer()->enlargeCanvas(this);

    for (CardList::Iterator it = myCards.begin(); it != myCards.end(); ++it)
    {
        (*it)->setVisible(vis);
        dealer()->enlargeCanvas(*it);
    }
}

void Pile::moveBy(double dx, double dy)
{
    QCanvasRectangle::moveBy(dx, dy);
    dealer()->enlargeCanvas(this);

    for (CardList::Iterator it = myCards.begin(); it != myCards.end(); ++it)
    {
        (*it)->moveBy(dx, dy);
        dealer()->enlargeCanvas(*it);
    }
}

int Pile::indexOf(const Card *c) const
{
    assert(c->source() == this);
    return myCards.findIndex(const_cast<Card*>(c)); // the list is of non-const cards
}

Card *Pile::at(int index) const
{
    if (index < 0 || index >= int(myCards.count()))
        return 0;
    return *myCards.at(index);
}

void Pile::clear()
{
    for (CardList::Iterator it = myCards.begin(); it != myCards.end(); ++it)
    {
        (*it)->setSource(0);
    }
    myCards.clear();
}

void Pile::add( Card *_card, int index)
{
    if (_card->source() == this)
        return;

    Pile *source = _card->source();
    if (source) {
        _card->setTakenDown(source->target() && !target());
        source->remove(_card);
    }

    _card->setSource(this);

    if (index == -1)
        myCards.append(_card);
    else {
        while (myCards.count() <= uint(index))
            myCards.append(0);
        assert(myCards[index] == 0);
        myCards[index] = _card;
    }
}

QSize Pile::cardOffset( bool _spread, bool _facedown, const Card *before) const
{
    if (_spread) {
        if (_facedown)
            return QSize(0, DSPREAD);
        else {
            if (before && !before->isFaceUp())
                return QSize(0, DSPREAD);
            else
                return QSize(0, SPREAD);
        }
    }

    return QSize(0, 0);
}

/* override cardtype (for initial deal ) */
void Pile::add( Card* _card, bool _facedown, bool _spread )
{
    if (!_card)
        return;

    Card *t = top();

    if (visible()) {
        _card->show();
    } else
        _card->hide();

    _card->turn( !_facedown );

    QSize offset = cardOffset(_spread, _facedown, t);

    int x2, y2, z2;

    if (t) {
        x2 = int(t->realX() + offset.width());
        y2 = int(t->realY() + offset.height());
        z2 = int(t->realZ() + 1);
    } else {
        x2 = int(x());
        y2 = int(y());
        z2 = int(z() + 1);
    }

    add(_card);

    if (_facedown) {
        _card->move( x2, y2 );
        _card->setZ( z2 );
    } else {
        _card->animatedMove(x2, y2, z2, 10);
    }



    dealer()->enlargeCanvas(_card);
}

void Pile::remove(Card *c)
{
    assert(myCards.contains(c));
    myCards.remove(c);
}

CardList Pile::cardPressed(Card *c)
{
    CardList result;

    if (!legalRemove(c))
        return result;

    int below = -1;

    if (!c->isFaceUp())
        return result;

    for (CardList::Iterator it = myCards.begin(); it != myCards.end(); ++it)
    {
        if (c == *it) {
            below = 0;
        }
        if (below >= 0) {
            (*it)->setZ(128 + below);
            below++;
            result.append(*it);
        }
    }
    return result;
}

void Pile::moveCards(CardList &cl, Pile *to)
{
    kdDebug() << "moveCards\n";

    if (!cl.count())
        return;

    for (CardList::Iterator it = cl.begin(); it != cl.end(); ++it)
        to->add(*it);

    if (removeFlags & autoTurnTop && top()) {
        Card *t = top();
        if (!t->isFaceUp()) {
            t->flipTo(t->x(), t->y(), 8);
            canvas()->update();
        }
    }

    to->moveCardsBack(cl, false);
}

void Pile::moveCardsBack(CardList &cl, bool anim)
{
    kdDebug() << "moveCardsBack\n";

    if (!cl.count())
        return;

    Card *c = cl.first();

    Card *before = 0;
    QSize off;

    int steps = 5;
    if (!anim)
        steps = 0;

    for (CardList::Iterator it = myCards.begin(); it != myCards.end(); ++it)
    {
        if (c == *it) {
            if (before) {
                off = cardOffset(addFlags & Pile::addSpread, false, before);
                c->animatedMove( before->realX() + off.width(),
                                 before->realY() + off.height(),
                                 before->realZ() + 1, steps);
                dealer()->enlargeCanvas(c);
            }
            else {
                c->animatedMove( x(), y(), z() + 1, steps);
            }
            break;
        } else
            before = *it;
    }

    before = c;
    CardList::Iterator it = cl.begin(); // == c
    ++it;

    off = cardOffset(addFlags & Pile::addSpread, false, 0);

    for (; it != cl.end(); ++it)
    {
        (*it)->animatedMove( before->realX() + off.width(),
                             before->realY() + off.height(),
                             before->realZ() + 1, steps);
        dealer()->enlargeCanvas(*it);
        before = *it;
    }
}

void Pile::cardClicked(Card *c)
{
    emit clicked(c);
}

void Pile::cardDblClicked(Card *c)
{
    emit dblClicked(c);
}

#include "pile.moc"
