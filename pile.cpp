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

#include <QtCore/QTimer>

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
      m_marked( false ),
      m_graphicVisible( true )
{
    if ( objectName.isEmpty() )
        setObjectName( QString("pile%1").arg(_index) );
    else
        setObjectName( objectName );

    QGraphicsItem::setVisible(true); // default


    setZValue(0);
    setSpread( 0, 0.33 );
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
            kDebug() << "pile doesn't match" << index() << " - " << i;
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

    const QSize cardSize = dscene()->cardDeck()->cardSize();

    QPointF new_pos = QPointF( _pilePos.x() * cardSize.width(),
                               _pilePos.y() * cardSize.height() );

    if ( new_pos.x() < 0 )
        new_pos.setX( dscene()->contentArea().width() - cardSize.width() + new_pos.x() );

    if ( new_pos.y() < 0 )
        new_pos.setY( dscene()->contentArea().height() - cardSize.height() + new_pos.y() );

    if ( new_pos != pos() )
    {
        setPos( new_pos );
        tryRelayoutCards();
    }

    if ( m_graphicVisible )
    {
        setPixmap( Render::renderElement( isMarked() ? "pile_selected" : "pile", cardSize ) );
    }
    else
    {
        QPixmap blank( cardSize );
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
    foreach ( Card *card, m_cards )
        card->setSource(0);
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

    if (_card->scene() != dscene())
        dscene()->addItem(_card);

    Pile *oldSource = _card->source();
    if (oldSource)
    {
        _card->setTakenDown(oldSource->target() && !target());
        oldSource->remove(_card);
    }

    _card->setSource(this);
    _card->setVisible( isVisible() );

    // If the card didn't have a source, its pixmap might be out of date.
    if (!oldSource)
        _card->updatePixmap();

    if (index == -1)
        m_cards.append(_card);
    else {
        while (m_cards.count() <= index)
            m_cards.append(0);
        Q_ASSERT(m_cards[index] == 0);
        m_cards[index] = _card;
    }
}


// Return the number of pixels in x and y that the card should be
// offset from the start position of the pile.
QPointF Pile::cardOffset( const Card *card ) const
{
    if ( addFlags & Pile::addSpread )
    {
        QPointF offset( spread().width() * dscene()->cardDeck()->cardWidth(),
                        spread().height() * dscene()->cardDeck()->cardHeight() );
        if (!card->realFace())
            offset *= 0.6;
        return offset;
    }

    return QPointF(0, 0);
}

/* override cardtype (for initial deal ) */
void Pile::animatedAdd( Card* _card, bool faceUp )
{
    if ( _card->source() )
        _card->source()->tryRelayoutCards();

    QPointF origPos = _card->pos();
    _card->turn( faceUp );
    add(_card);
    layoutCards( DURATION_RELAYOUT);

    _card->stopAnimation();
    QPointF destPos = _card->pos();
    _card->setPos( origPos );

    QPointF delta = destPos - _card->pos();
    qreal dist = sqrt( delta.x() * delta.x() + delta.y() * delta.y() );
    qreal whole = sqrt( scene()->width() * scene()->width() + scene()->height() * scene()->height() );
    _card->moveTo(destPos, _card->zValue(), qRound( dist * DURATION_DEAL / whole ) );

    if ( _card->animated() )
    {
        dscene()->setWaiting( true );
        connect(_card, SIGNAL(stopped(Card*)), SLOT(waitForMoving(Card*)));
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
    c->setSource(0);
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

void Pile::setMarked( bool flag )
{
    m_marked = flag;
    rescale();
}

bool Pile::isMarked() const
{
    return m_marked;
}

void Pile::setGraphicVisible( bool flag ) {
    m_graphicVisible = flag;
    rescale();
}


CardList Pile::cardPressed(Card *c)
{
    CardList result;

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
            (*it)->setZValue(1000 + below);
            below++;
            result.append(*it);
        }
    }
    return result;
}

void Pile::moveCards(CardList &cl, Pile *to)
{
    if (cl.isEmpty())
        return;

    foreach (Card * c, cl)
    {
        Q_ASSERT( c->source() == this );
        to->add(c);
    }

    Card *t = top();
    if (t && !t->isFaceUp() && removeFlags & autoTurnTop)
    {
        t->animate(t->pos(), t->zValue(), 1, 0, true, false, DURATION_FLIP);
    }

    relayoutCards();

    to->moveCardsBack(cl);
}

void Pile::moveCardsBack(CardList &cl, int duration)
{
    if (!cl.count())
        return;

    if ( duration == -1 )
        duration = DURATION_MOVEBACK;

    foreach ( Card *c, cl )
        c->raise();

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

    const QSize cardSize = dscene()->cardDeck()->cardSize();

    QPointF totalOffset( 0, 0 );
    foreach ( const Card *c, m_cards )
        totalOffset += cardOffset( c );

    qreal divx = 1;
    if ( totalOffset.x() )
        divx = qMin<qreal>( ( maximumSpace().width() - cardSize.width() ) / totalOffset.x(), 1.0 );

    qreal divy = 1;
    if ( totalOffset.y() )
        divy = qMin<qreal>( ( maximumSpace().height() - cardSize.height() ) / totalOffset.y(), 1.0 );

    QPointF cardPos = pos();
    qreal z = zValue() + 1;
    foreach ( Card * card, m_cards )
    {
        if ( duration )
        {
            card->animate( cardPos, z, 1, 0, card->isFaceUp(), false, dscene()->speedUpTime( duration ) );
        }
        else
        {
            card->setZValue( z );
            card->setPos( cardPos );
        }
        QPointF offset = cardOffset( card );
        cardPos.rx() += divx * offset.x();
        cardPos.ry() += divy * offset.y();
        ++z;
    }
}

#include "pile.moc"
