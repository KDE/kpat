/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 *
 * License of original code:
 * -------------------------------------------------------------------------
 *   Permission to use, copy, modify, and distribute this software and its
 *   documentation for any purpose and without fee is hereby granted,
 *   provided that the above copyright notice appear in all copies and that
 *   both that copyright notice and this permission notice appear in
 *   supporting documentation.
 *
 *   This file is provided AS IS with no warranties of any kind.  The author
 *   shall have no liability with respect to the infringement of copyrights,
 *   trade secrets or any patents by this file or any part thereof.  In no
 *   event will the author be liable for any lost revenue or profits or
 *   other special, indirect and consequential damages.
 * -------------------------------------------------------------------------
 *
 * License of modifications/additions made after 2009-01-01:
 * -------------------------------------------------------------------------
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of 
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * -------------------------------------------------------------------------
 */

#include "pile.h"

#include "dealer.h"
#include "carddeck.h"
#include "render.h"
#include "speeds.h"

#include <KDebug>

#include <cmath>


const int Pile::Default       = 0x0000;
const int Pile::disallow      = 0x0001;
const int Pile::several       = 0x0002; // default: move one card

// Add-flags
const int Pile::addSpread     = 0x0100;

// Remove-flags
const int Pile::autoTurnTop   = 0x0200;
const int Pile::wholeColumn   = 0x0400;
const int Pile::demoOK        = 0x0800;

Pile::Pile( int _index, const QString & objectName )
    : QGraphicsPixmapItem(),
      removeFlags(0),
      addFlags(0),
      _atype(Custom),
      _rtype(Custom),
      _checkIndex(-1),
      myIndex(_index),
      _target(false),
      m_highlighted( false ),
      m_graphicVisible( true )
{
    if ( objectName.isEmpty() )
        setObjectName( QString("pile%1").arg(_index) );
    else
        setObjectName( objectName );

    QGraphicsItem::setVisible(true); // default


    setZValue(0);
    setSpread( 0.33 );
    setReservedSpace( QSizeF( 1, 1 ) );
    setShapeMode( QGraphicsPixmapItem::BoundingRectShape );
    setMaximumSpace( QSizeF( 1, 1 ) ); // just to make it valid
    m_relayoutTimer = new QTimer( this );
    m_relayoutTimer->setSingleShot( true );
    connect( m_relayoutTimer, SIGNAL(timeout()), SLOT(relayoutCards()) );
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
//     dscene()->removePile(this);

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

void Pile::setPilePos( qreal x,  qreal y )
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

    QPointF new_pos = QPointF( _pilePos.x() * CardDeck::self()->cardWidth(),
                               _pilePos.y() * CardDeck::self()->cardHeight() );

    if ( new_pos.x() < 0 )
        new_pos.setX( dscene()->contentArea().width() - CardDeck::self()->cardWidth() + new_pos.x() );

    if ( new_pos.y() < 0 )
        new_pos.setY( dscene()->contentArea().height() - CardDeck::self()->cardHeight() + new_pos.y() );

    if ( new_pos != pos() )
    {
        setPos( new_pos );
        tryRelayoutCards();
    }

    QSize size = CardDeck::self()->cardSize();
    if ( m_graphicVisible )
    {
        setPixmap( Render::renderElement( isHighlighted() ? "pile_selected" : "pile", size ) );
    }
    else
    {
        QPixmap blank( size );
        blank.fill( Qt::transparent );
        setPixmap( blank );
    }
}

bool Pile::legalAdd( const CardList& _cards, bool ) const
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

bool Pile::legalRemove(const Card *c, bool demo) const
{
    if ( demo && removeFlags & demoOK )
        return true;

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
    if ( vis == isVisible() )
        return;

    QGraphicsItem::setVisible(vis);
    foreach (Card *c, m_cards)
        c->setVisible(vis);
}

int Pile::indexOf(const Card *c) const
{
    Q_ASSERT(c->source() == this);
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
    m_relayoutTimer->stop();

    foreach ( Card * card, m_cards )
    {
        if ( card->animated() || dscene()->isMoving( card ) ) {
            tryRelayoutCards();
            return;
        }
    }

    layoutCards( dscene()->speedUpTime( DURATION_RELAYOUT ) );
}

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
        Q_ASSERT(m_cards[index] == 0);
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
        // kDebug(11111) << "::add" << t->pos() << " " << t->spread() << " " << _card->name() << " " << t->name() << " " << _card->spread();
        x2 = t->realX() + t->spread().width() * CardDeck::self()->cardWidth();
        y2 = t->realY() + t->spread().height() * CardDeck::self()->cardHeight();
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
        if ( source == CardDeck::self() && dscene()->isInitialDeal() )
        {
            _card->setPos(QPointF( x2, 0 - 3 * CardDeck::self()->cardHeight() ) );
        }
        _card->setZValue( z2 );
        qreal distx = x2 - _card->x();
        qreal disty = y2 - _card->y();
        qreal dist = sqrt( distx * distx + disty * disty );
        qreal whole = sqrt( scene()->width() * scene()->width() + scene()->height() * scene()->height() );
        _card->moveTo(x2, y2, z2, qRound( dist * 1000 / whole ) );

        if ( _card->animated() )
        {
            dscene()->setWaiting( true );
            connect(_card, SIGNAL(stopped(Card*)), SLOT(waitForMoving(Card*)));
        }
    }
}

void Pile::waitForMoving( Card*c )
{
    c->disconnect(this);
    dscene()->setWaiting( false );
}

void Pile::remove(Card *c)
{
    Q_ASSERT(m_cards.contains(c));
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

void Pile::setGraphicVisible( bool flag ) {
    m_graphicVisible = flag;
    rescale();
}


CardList Pile::cardPressed(Card *c)
{
    CardList result;

    //kDebug(11111) << gettime() << "cardPressed" << c->name() << " " << c->isFaceUp();
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
            t->flipTo(t->x(), t->y(), DURATION_FLIP );
        }
    }

    to->moveCardsBack(cl);
}

void Pile::moveCardsBack(CardList &cl, int duration)
{
    if (!cl.count())
        return;

    if ( duration == -1 )
        duration = DURATION_MOVEBACK;

    layoutCards( duration );
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
    if ( dscene() && dscene()->speedUpTime( 40 ) < 30 )
        m_relayoutTimer->start( 400 );
    else
        m_relayoutTimer->start( 40 );
}

void Pile::layoutCards(int duration)
{
    if ( m_cards.isEmpty() )
        return;

    QSizeF preferredSize( 0, 0 );
    for ( int i = 0; i < m_cards.size() - 1; ++i )
    {
        // The spreads should hopefully have all one sign, or we get in trouble
        preferredSize.rwidth() += qAbs( m_cards[i]->spread().width() );
        preferredSize.rheight() += qAbs( m_cards[i]->spread().height() );
    }

    qreal divx = 1;
    if ( preferredSize.width() )
        divx = qMin<qreal>( ( maximumSpace().width() - CardDeck::self()->cardWidth() ) / ( preferredSize.width() * CardDeck::self()->cardWidth() ), 1.0 );

    qreal divy = 1;
    if ( preferredSize.height() )
        divy = qMin<qreal>( ( maximumSpace().height() - CardDeck::self()->cardHeight() ) / ( preferredSize.height() * CardDeck::self()->cardHeight() ), 1.0 );

    QPointF cardPos = pos();
    qreal z = zValue() + 1;
    foreach ( Card * card, m_cards )
    {
        card->moveTo( cardPos.x(), cardPos.y(), z, dscene()->speedUpTime( duration ) );
        cardPos.rx() += divx * card->spread().width() * CardDeck::self()->cardWidth();
        cardPos.ry() += divy * card->spread().height() * CardDeck::self()->cardHeight();
        z += 1;
    }
}

#include "pile.moc"
