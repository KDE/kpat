#include "pile.h"
#include "dealer.h"
#include "card.h"
#include <kdebug.h>
#include <qpainter.h>
#include "cardmaps.h"
#include <qdrawutil.h>
#include <kapp.h>
#include <assert.h>

const int Pile::Default       = 0x0000;
const int Pile::disallow      = 0x0001;
const int Pile::several       = 0x0002; // default: move one card
const int Pile::faceDown      = 0x0004; //  move/add cards facedown

// Add-flags
const int Pile::addSpread     = 0x0100;
const int Pile::addRotated    = 0x0600; // Note: cannot have Spread && Rotate

// Remove-flags
const int Pile::autoTurnTop   = 0x0200;
const int Pile::wholeColumn   = 0x0400;

const int SPREAD = 20;
const int DSPREAD = 5;

Card *Pile::top() const
{
    if (myCards.isEmpty())
        return 0;

    return myCards.last();
}

Pile::Pile( int _index, Dealer* parent)
    : QCanvasRectangle( parent->canvas() ), _dealer(parent), myIndex(_index), direction(0)
{
    setSize( cardMap::CARDX, cardMap::CARDY );
    removeCheckFun = 0;
    addCheckFun = 0;
    addFlags = removeFlags = 0;
    setBrush(Qt::black);
    setPen(QPen(Qt::black));
    setZ(0);
}

Pile::~Pile()
{
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

static QColorGroup colgrp( Qt::black, Qt::white, Qt::darkGreen.light(),
                           Qt::darkGreen.dark(), Qt::darkGreen, Qt::black,
                           Qt::white );

void Pile::drawShape ( QPainter & painter )
{
    QColor green = Qt::darkGreen.dark();
    if (selected()) {
        green = green.light(300);
    }
    painter.fillRect( int(x()), int(y()), width(), height(), green);         // initialize pixmap
    kapp->style().drawPanel( &painter, int(x()), int(y()), cardMap::CARDX, cardMap::CARDY, colgrp, true );
}

bool Pile::legalAdd( const CardList& _card ) const
{
    if( addFlags & disallow ) {
        kdDebug() << "no adding allowed\n";
        return false;
    }

    if (!legalMoves.isEmpty() && !legalMoves.contains(_card.first()->source()->index()))
    {
        kdDebug() << "illegal source: " << _card.first()->source()->index() << "  "
                  << legalMoves.count() << endl;
        return false;
    }

    if( !( addFlags & several ) &&
        _card.count() > 1 )
    {
        kdDebug() << "several not allowed\n";
        return false;
    }

    if( addCheckFun ) {
        bool result = addCheckFun( this, _card );
        kdDebug() << "addCheckFun returned " << result << endl;
        return result;
    }

    kdDebug() << "legalAdd\n";
    return true;
}

bool Pile::legalRemove(const Card *c) const
{
    kdDebug() << "legalRemove\n";

    if( removeFlags & disallow )
        return false;

    if( !( removeFlags & several ) && myCards.find(const_cast<Card*>(c)) != myCards.end() )
        return false;

    if( removeCheckFun ) {
        bool b = removeCheckFun( this, c);
        kdDebug() << "removeCheckFun returned " << b << endl;
        return b;
    }
    return true;
}

void Pile::setVisible(bool vis)
{
    QCanvasRectangle::setVisible(vis);
    Card::enlargeCanvas(this);

    for (CardList::Iterator it = myCards.begin(); it != myCards.end(); ++it)
    {
        (*it)->setVisible(vis);
        Card::enlargeCanvas(*it);
    }
}

void Pile::moveBy(double dx, double dy)
{
    QCanvasRectangle::moveBy(dx, dy);
    Card::enlargeCanvas(this);

    for (CardList::Iterator it = myCards.begin(); it != myCards.end(); ++it)
    {
        (*it)->moveBy(dx, dy);
        Card::enlargeCanvas(*it);
    }
}

/* override cardtype (for initial deal ) */
void Pile::add( Card* _card, bool _facedown, bool _spread )
{
    if (_card->source() && _card->source() != this) {
        _card->source()->remove(_card);
    }
    _card->setSource(this);
    Card *t = top();
    myCards.append(_card);

    if (visible()) {
        _card->show();
    } else
        _card->hide();

    _card->turn( !_facedown );

    int off = 0;
    if (_spread) {
        if (_facedown)
            off = DSPREAD;
        else {
            if (t && !t->isFaceUp())
                off = DSPREAD;
            else
                off = SPREAD;
        }
    }
    if (t) {
        _card->move( t->x(), t->y() + off );
        _card->setZ( t->z() + 1);
    } else {
        _card->move( x(), y() );
        _card->setZ( z() + 1);
    }
    Card::enlargeCanvas(_card);
}

void Pile::remove(Card *c)
{
    myCards.remove(c);
}

void Pile::clear()
{
    myCards.clear();
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
    if (!cl.count())
        return;

    Card *c = cl.first();
    bool gotit = false;

    for (CardList::Iterator it = myCards.begin(); it != myCards.end(); )
    {
        if (c == *it) // everything below the first moved card is moved
            gotit = true;
        if (gotit) {
            (*it)->setSource(0);
            it = myCards.remove(it);
        } else
            ++it;
    }
    if (removeFlags & autoTurnTop && top()) {
        Card *t = top();
        if (!t->isFaceUp()) {
            t->flipTo(t->x(), t->y(), 8);
            canvas()->update();
        }
    }

    for (CardList::Iterator it = cl.begin(); it != cl.end(); ++it)
    {
        (*it)->setSource(to);
    }

    to->myCards += cl;
    to->moveCardsBack(cl);
}

void Pile::moveCardsBack(CardList &cl)
{
    if (!cl.count())
        return;

    Card *c = cl.first();

    Card *before = 0;
    int off = 0;

    for (CardList::Iterator it = myCards.begin(); it != myCards.end(); ++it)
    {
        if (c == *it) {
            if (before) {

                if (addFlags & Pile::addSpread) {
                    if (!before->isFaceUp())
                        off = DSPREAD;
                    else
                        off = SPREAD;
                }

                c->setZ(before->z() + 1);
                c->move( before->x(), before->y() + off );
            }
            else {
                c->setZ( z() + 1);
                c->move( x(), y() );
            }
            break;
        } else
            before = *it;
    }

    before = c;
    CardList::Iterator it = cl.begin(); // == c
    ++it;

    off = 0;
    if (addFlags & Pile::addSpread)
        off = SPREAD;

    for (; it != cl.end(); ++it)
    {
        (*it)->move( before->x(), before->y() + off );
        (*it)->setZ( before->z() + 1);
        before = *it;
    }
}

