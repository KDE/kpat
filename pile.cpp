#include "pile.h"
#include "dealer.h"
#include <kdebug.h>
#include <qpainter.h>
#include "cardmaps.h"
#include <assert.h>
#include "speeds.h"

const int Pile::RTTI          = 1002;

const int Pile::Default       = 0x0000;
const int Pile::disallow      = 0x0001;
const int Pile::several       = 0x0002; // default: move one card

// Add-flags
const int Pile::addSpread     = 0x0100;

// Remove-flags
const int Pile::autoTurnTop   = 0x0200;
const int Pile::wholeColumn   = 0x0400;



Pile::Pile( int _index, Dealer* _dealer)
    : QCanvasRectangle( _dealer->canvas() ),
      m_dealer(_dealer),
      m_atype(Custom),
      m_rtype(Custom),
      myIndex(_index),
      _target(false)
{
    // Make the patience aware of this pile.
    dealer()->addPile(this);

    QCanvasRectangle::setVisible(true); // default
    _checkIndex = -1;
    m_addFlags    = 0;
    m_removeFlags = 0;

    setBrush(Qt::black);
    setPen(QPen(Qt::black));

    setZ(0);
    initSizes();
}


void Pile::initSizes()
{
    setSpread( cardMap::CARDY() / 5 + 1 );
    setHSpread( cardMap::CARDX() / 9 + 1 );
    setDSpread( cardMap::CARDY() / 8 );

    setSize( cardMap::CARDX(), cardMap::CARDY() );
}

void Pile::setType(PileType type)
{
    setAddType(type);
    setRemoveType(type);
}

void Pile::setAddType(PileType _type)
{
    m_atype = _type;
    switch (_type) {
        case Custom:
        case FreeCell:
            break;
        case KlondikeTarget:
            setTarget(true);
            break;
        case KlondikeStore:
        case GypsyStore:
        case FreecellStore:
            setAddFlags(Pile::addSpread | Pile::several);
            break;
    }
}

void Pile::setRemoveType(PileType _type)
{
    m_rtype = _type;
    switch (_type) {
        case Custom:
            break;
        case KlondikeTarget:
            setRemoveFlags(Pile::disallow);
            break;
        case KlondikeStore:
        case GypsyStore:
        case FreeCell:
            break;
        case FreecellStore:
            setRemoveFlags(Pile::several | Pile::autoTurnTop);
            break;
    }
}

Pile::~Pile()
{
    dealer()->removePile(this);

    for (CardList::Iterator it = m_cards.begin(); it != m_cards.end(); ++it)
    {
        if ((*it)->source() != this) {
            int i = -13;
            if ((*it)->source())
                i = (*it)->source()->index();
            kdDebug(11111) << "pile doesn't match " << index() << " - " << i << endl;
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
    if (isSelected()) {
        if (cache.isNull())
            dealer()->drawPile(cache, this, false);
        painter.drawPixmap(int(x()), int(y()), cache);
    } else {
        if (cache_selected.isNull())
            dealer()->drawPile(cache_selected, this, true);
        painter.drawPixmap(int(x()), int(y()), cache_selected);
    }
}

bool Pile::legalAdd( const CardList& _cards ) const
{
    if ( m_addFlags & disallow )
        return false;

    if ( !( m_addFlags & several ) && _cards.count() > 1 )
        return false;

    // getHint removes cards without turning, so it could be it
    // checks later if cards can be added to a face down card
    if (top() && !top()->realFace())
	return false;

    switch (addType()) {
        case Custom:
            return dealer()->checkAdd( checkIndex(), this, _cards );
            break;
        case KlondikeTarget:
            return add_klondikeTarget(_cards);
            break;
        case FreecellStore:
        case KlondikeStore:
            return add_klondikeStore(_cards);
            break;
        case GypsyStore:
            return add_gypsyStore(_cards);
            break;
        case FreeCell:
            return add_freeCell(_cards);
    }
    return false;
}

bool Pile::legalRemove(const Card *c) const
{
    if ( m_removeFlags & disallow ) {
        return false;
    }
    if ( !( m_removeFlags & several ) && top() != c)
	return false;

    switch (removeType()) {
        case Custom:
            return dealer()->checkRemove( checkIndex(), this, c);
            break;
        case KlondikeTarget:
        case GypsyStore:
        case KlondikeStore:
            break;
        case FreecellStore:
            return remove_freecellStore(c);
            break;
        case FreeCell:
            return (top() == c);
            break;
    }
    return true;
}

void Pile::setVisible(bool vis)
{
    QCanvasRectangle::setVisible(vis);
    dealer()->enlargeCanvas(this);

    for (CardList::Iterator it = m_cards.begin(); it != m_cards.end(); ++it)
    {
        (*it)->setVisible(vis);
        dealer()->enlargeCanvas(*it);
    }
}

void Pile::moveBy(double dx, double dy)
{
    QCanvasRectangle::moveBy(dx, dy);
    dealer()->enlargeCanvas(this);

    for (CardList::Iterator it = m_cards.begin(); it != m_cards.end(); ++it)
    {
        (*it)->moveBy(dx, dy);
        dealer()->enlargeCanvas(*it);
    }
}

int Pile::indexOf(const Card *c) const
{
    assert(c->source() == this);
    return m_cards.findIndex(const_cast<Card*>(c)); // the list is of non-const cards
}

Card *Pile::at(int index) const
{
    if (index < 0 || index >= int(m_cards.count()))
        return 0;
    return *m_cards.at(index);
}

// Return the top card of this pile.
//

Card *Pile::top() const
{
    if (m_cards.isEmpty())
        return 0;

    return m_cards.last();
}

void Pile::clear()
{
    for (CardList::Iterator it = m_cards.begin(); it != m_cards.end(); ++it)
    {
        (*it)->setSource(0);
    }
    m_cards.clear();
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
        m_cards.append(_card);
    else {
        while (m_cards.count() <= uint(index))
            m_cards.append(0);
        assert(m_cards[index] == 0);
        m_cards[index] = _card;
    }
}


// Return the number of pixels in x and y that the card should be
// offset from the start position of the pile. 
//
// Note: Default is to only have vertical spread (Y direction). 

QSize Pile::cardOffset( bool _spread, bool _facedown, const Card *before) const
{
    if (_spread) {
        if (_facedown)
            return QSize(0, dspread());
        else {
            if (before && !before->isFaceUp())
                return QSize(0, dspread());
            else
                return QSize(0, spread());
        }
    }

    return QSize(0, 0);
}

/* override cardtype (for initial deal ) */
void Pile::add( Card* _card, bool _facedown, bool _spread )
{
    if (!_card)
        return;

    // The top card
    Card *t = top();

    // If this pile is visible, then also show the card.
    if (isVisible())
        _card->show();
    else
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

    if (_facedown || !isVisible()) {
        _card->move( x2, y2 );
        _card->setZ( z2 );
    } else {
        _card->moveTo(x2, y2, z2, STEPS_INITIALDEAL);
    }

    dealer()->enlargeCanvas(_card);
}

void Pile::remove(Card *c)
{
    assert(m_cards.contains(c));
    m_cards.remove(c);
}

void Pile::hideCards( const CardList & cards )
{
    for (CardList::ConstIterator it = cards.begin(); it != cards.end(); ++it)
        m_cards.remove(*it);
}

void Pile::unhideCards( const CardList & cards )
{
    for (CardList::ConstIterator it = cards.begin(); it != cards.end(); ++it)
        m_cards.append(*it);
}

CardList Pile::cardPressed(Card *c)
{
    CardList result;

    if (!legalRemove(c))
        return result;

    int below = -1;

    if (!c->isFaceUp())
        return result;

    for (CardList::Iterator it = m_cards.begin(); it != m_cards.end(); ++it)
    {
        if (c == *it) {
            below = 0;
        }
        if (below >= 0) {
            (*it)->setAnimated(false);
            (*it)->setZ(128 + below);
            below++;
            result.append(*it);
        }
    }
    return result;
}

void Pile::moveCards(CardList &cl, Pile *to)
{
    if (!cl.count())
        return;

    for (CardList::Iterator it = cl.begin(); it != cl.end(); ++it)
        to->add(*it);

    if (m_removeFlags & autoTurnTop && top()) {
        Card *t = top();
        if (!t->isFaceUp()) {
            t->flipTo(int(t->x()), int(t->y()), 8);
            canvas()->update();
        }
    }

    to->moveCardsBack(cl, false);
}

void Pile::moveCardsBack(CardList &cl, bool anim)
{
    if (!cl.count())
        return;

    Card *c = cl.first();

    Card *before = 0;
    QSize off;

    int steps = STEPS_MOVEBACK;
    if (!anim)
        steps = 0;

    for (CardList::Iterator it = m_cards.begin(); it != m_cards.end(); ++it)
    {
        if (c == *it) {
            if (before) {
                off = cardOffset(m_addFlags & Pile::addSpread, false, before);
                c->moveTo( before->realX() + off.width(),
			   before->realY() + off.height(),
			   before->realZ() + 1, steps);
                dealer()->enlargeCanvas(c);
            }
            else {
                c->moveTo( int(x()), int(y()), int(z()) + 1, steps);
            }
            break;
        } else
            before = *it;
    }

    before = c;
    CardList::Iterator it = cl.begin(); // == c
    ++it;

    off = cardOffset(m_addFlags & Pile::addSpread, false, 0);

    for (; it != cl.end(); ++it)
    {
        (*it)->moveTo( before->realX() + off.width(),
                             before->realY() + off.height(),
                             before->realZ() + 1, steps);
        dealer()->enlargeCanvas(*it);
        before = *it;
    }
}

bool Pile::cardClicked(Card *c)
{
    emit clicked(c);
    return false;
}

bool Pile::cardDblClicked(Card *c)
{
    emit dblClicked(c);
    return false;
}

#include "pile.moc"
