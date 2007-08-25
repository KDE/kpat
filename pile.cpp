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
#include "pile.h"
#include "dealer.h"
#include <kdebug.h>
#include <qpainter.h>
#include "cardmaps.h"
#include <kstandarddirs.h>
#include <deck.h>
#include <assert.h>
#include "speeds.h"
#include <QSvgRenderer>
#include <math.h>

const int Pile::my_type       = DealerScene::PileTypeId;

const int Pile::Default       = 0x0000;
const int Pile::disallow      = 0x0001;
const int Pile::several       = 0x0002; // default: move one card

// Add-flags
const int Pile::addSpread     = 0x0100;

// Remove-flags
const int Pile::autoTurnTop   = 0x0200;
const int Pile::wholeColumn   = 0x0400;

QPixmap *Pile::cache = 0;
QPixmap *Pile::cache_selected = 0;

Pile::Pile( int _index, DealerScene* parent)
    : QGraphicsPixmapItem( ),
      _atype(Custom),
      _rtype(Custom),
      myIndex(_index),
      _target(false),
      m_highlighted( false )
{
    setObjectName( "<unknown>" );
    parent->addItem( this );

    // Make the patience aware of this pile.
    dscene()->addPile(this);

    QGraphicsItem::setVisible(true); // default
    _checkIndex = -1;
    addFlags    = 0;
    removeFlags = 0;

    setZValue(0);
    setSpread( 3.3 );
    setReservedSpace( QSizeF( 10, 10 ) );
    setShapeMode( QGraphicsPixmapItem::BoundingRectShape );
    setMaximalSpace( QSizeF( 1, 1 ) ); // just to make it valid
    m_relayoutTimer = new QTimer( this );
    m_relayoutTimer->setSingleShot( true );
    connect( m_relayoutTimer, SIGNAL( timeout() ), SLOT( relayoutCards() ) );

    if ( !cache )
        cache = new QPixmap( KStandardDirs::locateLocal( "data", "kpat/pile.png" ) );
    if ( !cache_selected )
        cache_selected = new QPixmap( KStandardDirs::locateLocal( "data", "kpat/pile_selected.png" ) );
}

QSvgRenderer *Pile::_renderer = 0;

QSvgRenderer *Pile::pileRenderer()
{
    if ( !_renderer )
        _renderer = new QSvgRenderer( KStandardDirs::locate( "data", "kpat/pile.svg" ) );
    return _renderer;
}

DealerScene *Pile::dscene() const
{
    return dynamic_cast<DealerScene*>( scene() );
}

void Pile::setType(PileType type)
{
    setAddType(type);
    setRemoveType(type);
}

void Pile::setAddType(PileType type)
{
    _atype = type;
    switch (type) {
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

void Pile::setRemoveType(PileType type)
{
    _rtype = type;
    switch (type) {
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
    dscene()->removePile(this);

    for (CardList::Iterator it = m_cards.begin(); it != m_cards.end(); ++it)
    {
        if ((*it)->source() != this) {
            int i = -13;
            if ((*it)->source())
                i = (*it)->source()->index();
            kDebug(11111) << "pile doesn't match" << index() << " - " << i;
        }
        (*it)->setSource(0);
    }
    delete m_relayoutTimer;
}

void Pile::setPilePos( double x,  double y )
{
    _pilePos = QPointF( x, y );
    rescale();
}

QPointF Pile::pilePos() const
{
    return _pilePos;
}

void Pile::rescale()
{
    if (!scene())
        return;

    int wantedheight = int( cardMap::self()->wantedCardHeight() + 1 );
    QPixmap *mycache = cache;
    if ( isHighlighted() )
        mycache = cache_selected;

    QPointF new_pos = QPointF( _pilePos.x() * cardMap::self()->wantedCardWidth() / 10.,
                               _pilePos.y() * cardMap::self()->wantedCardHeight() / 10. );
    //kDebug() << scene()->sceneRect() << " " << new_pos;
    if ( new_pos.x() < 0 )
        new_pos.setX( scene()->width() - cardMap::self()->wantedCardWidth() + new_pos.x() );
    if ( new_pos.y() < 0 )
        new_pos.setY( scene()->height() - cardMap::self()->wantedCardHeight() + new_pos.y() );

    if ( new_pos != pos() )
    {
        setPos( new_pos );
        tryRelayoutCards();
    }

    if ( mycache->height() == wantedheight )
    {
        setPixmap( *mycache );
        return;
    }
    kDebug(11111) << gettime() << "rescale start\n";

#if 0
    QImage pix( ( int )maximalSpace().width(),
                ( int )maximalSpace().height(),
                QImage::Format_ARGB32 );
    pix.fill( qRgba( 0, 0, 255, 255 ) );
#else
    QImage pix( static_cast<int>( cardMap::self()->wantedCardWidth() + 1 ),
                static_cast<int>( cardMap::self()->wantedCardHeight() + 1),
                QImage::Format_ARGB32 );
    pix.fill( qRgba( 0, 0, 255, 0 ) );
#endif
    QPainter p( &pix );

    pileRenderer()->render( &p, isHighlighted() ? "pile_selected" : "pile",
                            QRectF( 0, 0, cardMap::self()->wantedCardWidth(),
                                    cardMap::self()->wantedCardHeight() ) );
    p.end();
    *mycache = QPixmap::fromImage( pix );
    if ( isHighlighted() )
        pix.save( KStandardDirs::locateLocal( "data", "kpat/pile_selected.png" ), "PNG" );
    else
        pix.save( KStandardDirs::locateLocal( "data", "kpat/pile.png" ), "PNG" );
    setPixmap( *mycache );
    kDebug(11111) << gettime() << "rescale end\n";
}

bool Pile::legalAdd( const CardList& _cards ) const
{
    if( addFlags & disallow ) {
        return false;
    }

    if( !( addFlags & several ) &&
        _cards.count() > 1 )
    {
        return false;
    }

    // getHint removes cards without turning, so it could be it
    // checks later if cards can be added to a face down card
    if (top() && !top()->realFace())
	return false;

    switch (addType()) {
        case Custom:
            return dscene()->checkAdd( checkIndex(), this, _cards );
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
    if( removeFlags & disallow ) {
        return false;
    }
    if( !( removeFlags & several ) && top() != c)
    {
        return false;
    }

    switch (removeType()) {
        case Custom:
            return dscene()->checkRemove( checkIndex(), this, c);
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
    QGraphicsItem::setVisible(vis);

    for (CardList::Iterator it = m_cards.begin(); it != m_cards.end(); ++it)
        (*it)->setVisible(vis);

    dscene()->relayoutPiles();
}

int Pile::indexOf(const Card *c) const
{
    assert(c->source() == this);
    return m_cards.indexOf(const_cast<Card*>(c)); // the list is of non-const cards
}

Card *Pile::at(int index) const
{
    if (index < 0 || index >= int(m_cards.count()))
        return 0;
    return m_cards.at(index);
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

void Pile::relayoutCards()
{
    //kDebug() << gettime() <<  "relayoutCards";
    m_relayoutTimer->stop();

    QPointF mypos = pos();
    qreal z = zValue() + 1;
    QSizeF preferredSize = QSizeF( 0, 0 );

    for (CardList::Iterator it = m_cards.begin(); it != m_cards.end(); ++it)
    {
        //kDebug() << ( *it )->name() << " " << ( *it )->spread();
        // the spreads should hopefully have all one sign, or we get in trouble
        preferredSize.rwidth() += fabs( ( *it )->spread().width() );
        preferredSize.rheight() += fabs( ( *it )->spread().height() );
        if ( ( *it )->animated() || dscene()->isMoving( *it ) ) {
            // kDebug() << ( *it )->name() << "is animated - restarting timer\n";
            tryRelayoutCards();
            return;
        }
    }
    qreal divy = 1;
    qreal divx = 1;
    if ( preferredSize.height() )
        divy = qMin( ( maximalSpace().height() - cardMap::self()->wantedCardHeight() ) / preferredSize.height() * 10 / cardMap::self()->wantedCardHeight(), 1. );
    if ( preferredSize.width() )
        divx = qMin( ( maximalSpace().width() - cardMap::self()->wantedCardWidth() ) / preferredSize.width() * 10 / cardMap::self()->wantedCardWidth(), 1. );

    //kDebug() << "relayoutCards" << objectName() << " " <<  divx << " " << divy << " " << maximalSpace() << " " << preferredSize << " " << reservedSpace();

    for (CardList::Iterator it = m_cards.begin(); it != m_cards.end(); ++it)
    {
        // ( *it )->stopAnimation();
        ( *it )->moveTo( mypos.x(), mypos.y(), z, int( 120 * dscene()->autoDropFactor() ) );
        ( *it )->setZValue( z );
        mypos.rx() += ( *it )->spread().width() * divx / 10 * cardMap::self()->wantedCardWidth();
        mypos.ry() += ( *it )->spread().height() * divy / 10 * cardMap::self()->wantedCardHeight();
        z += 1;
    }
}

#include <sys/time.h>
#include <stdio.h>

void Pile::add( Card *_card, int index)
{
    if (_card->source() == this)
        return;

    Pile *source = _card->source();
    if (source) {
        _card->setTakenDown(source->target() && !target());
        source->remove(_card);
        source->tryRelayoutCards();
        _card->setVisible( isVisible() );
    }

    _card->setSource(this);

    QSizeF offset = cardOffset( _card );
    _card->setSpread( offset );

    if (index == -1)
        m_cards.append(_card);
    else {
        while (m_cards.count() <= index)
            m_cards.append(0);
        assert(m_cards[index] == 0);
        m_cards[index] = _card;
    }

    tryRelayoutCards();
}


// Return the number of pixels in x and y that the card should be
// offset from the start position of the pile.
//
// Note: Default is to only have vertical spread (Y direction).

QSizeF Pile::cardOffset( const Card *card ) const
{
    if ( addFlags & Pile::addSpread )
    {
        if (card->realFace())
            return QSizeF(0, spread());
        else
            return QSizeF(0, qMin( spread() * 0.6, 1.3) );
    }

    return QSize(0, 0);
}

/* override cardtype (for initial deal ) */
void Pile::add( Card* _card, bool _facedown )
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

    qreal x2, y2, z2;

    if (t) {
        // kDebug() << "::add" << t->pos() << " " << t->spread() << " " << _card->name() << " " << t->name() << " " << _card->spread();
        x2 = t->realX() + t->spread().width()  / 10 * cardMap::self()->wantedCardWidth();
        y2 = t->realY() + t->spread().height() / 10 * cardMap::self()->wantedCardHeight();
        z2 = t->realZ() + 1;
    } else {
        x2 = x();
        y2 = y();
        z2 = zValue() + 1;
    }

    Pile *source = _card->source();

    add(_card);

    bool face = _facedown;

#if 0
    if ( source == Deck::deck() ) // ignore then
        face = false;
#endif
    if (face || !isVisible()) {
        _card->setPos( QPointF( x2, y2 ) );
        _card->setZValue( z2 );
    } else {
        if ( source == Deck::deck() && dscene()->isInitialDeal() )
        {
            _card->setPos(QPointF( x2, 0 - 3 * cardMap::self()->wantedCardHeight() ) );
        }
        _card->setZValue( z2 );
        qreal distx = x2 - _card->x();
        qreal disty = y2 - _card->y();
        qreal dist = sqrt( distx * distx + disty * disty );
        qreal whole = sqrt( scene()->width() * scene()->width() + scene()->height() * scene()->height() );
        _card->moveTo(x2, y2, z2, qRound( dist * 1000 / whole ) );
    }
}

void Pile::remove(Card *c)
{
    assert(m_cards.contains(c));
    m_cards.removeAll(c);
}

void Pile::hideCards( const CardList & cards )
{
    for (CardList::ConstIterator it = cards.begin(); it != cards.end(); ++it)
        m_cards.removeAll(*it);
}

void Pile::unhideCards( const CardList & cards )
{
    for (CardList::ConstIterator it = cards.begin(); it != cards.end(); ++it)
        m_cards.append(*it);
}

void Pile::setHighlighted( bool flag ) {
    m_highlighted = flag;
    rescale();
}

CardList Pile::cardPressed(Card *c)
{
    CardList result;

    //kDebug() << gettime() << "cardPressed" << c->name() << " " << c->isFaceUp();
    emit pressed( c );

    if ( !legalRemove(c) )
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
            (*it)->stopAnimation();
            (*it)->setZValue(128 + below);
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

    if (removeFlags & autoTurnTop && top()) {
        Card *t = top();
        if (!t->isFaceUp()) {
            t->flipTo(t->x(), t->y());
        }
    }

    to->moveCardsBack(cl, true);
}

void Pile::moveCardsBack(CardList &cl, bool anim)
{
    if (!cl.count())
        return;

    Card *c = cl.first();

    Card *before = 0;
    QSizeF off;

    int steps = DURATION_MOVEBACK;
    if (!anim)
        steps = 0;

    for (CardList::Iterator iti = m_cards.begin(); iti != m_cards.end(); ++iti)
    {
        if (c == *iti) {

            if (before) {
                // kDebug() << "moveCardsBack" << ( *iti )->name() << " " << before->name() << " " << before->spread();
                off = before->spread();
                c->moveTo( before->realX() + off.width() / 10 * cardMap::self()->wantedCardWidth(),
			   before->realY() + off.height()  / 10 * cardMap::self()->wantedCardHeight(),
			   before->realZ() + 1, steps);
            }
            else {
                // kDebug() << "moveCardsBack" << ( *iti )->name() << "no before";
                c->moveTo( x(), y(), zValue() + 1, steps);
            }
            break;
        } else
            before = *iti;
    }

    before = c;
    CardList::Iterator it = cl.begin(); // == c
    ++it;

    for (; it != cl.end(); ++it)
    {
        off = before->spread();
        (*it)->moveTo( before->realX() + off.width() / 10 * cardMap::self()->wantedCardWidth(),
                       before->realY() + off.height()  / 10 * cardMap::self()->wantedCardHeight(),
                       before->realZ() + 1, steps);
        before = *it;
    }

    relayoutCards();
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

void Pile::tryRelayoutCards()
{
    m_relayoutTimer->start( int( 40 * dscene()->autoDropFactor() ) );
}

#include "pile.moc"
