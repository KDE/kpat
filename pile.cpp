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

#include "carddeck.h"
#include "cardscene.h"
#include "render.h"

#include <KDebug>

#include <QtCore/QTimer>
#include <QtCore/QPropertyAnimation>
#include <QtGui/QPainter>

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

    m_fadeAnimation = new QPropertyAnimation( this, "highlightedness", this );
    m_fadeAnimation->setDuration( DURATION_CARDHIGHLIGHT );
    m_fadeAnimation->setKeyValueAt( 0, 0 );
    m_fadeAnimation->setKeyValueAt( 1, 1 );
}

CardScene *Pile::cardScene() const
{
    return dynamic_cast<CardScene*>( scene() );
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
}

QPointF Pile::pilePos() const
{
    return _pilePos;
}

void Pile::updatePixmap()
{
    if ( !scene() )
        return;

    QSize size = cardScene()->deck()->cardSize();
    QPixmap pix( size );
    pix.fill( Qt::transparent );

    if ( m_graphicVisible )
    {
        if ( m_fadeAnimation->state() == QAbstractAnimation::Running )
        {
            QPainter p( &pix );
            p.setOpacity( 1 - m_highlightedness );
            p.drawPixmap( 0, 0, Render::renderElement( "pile", size ) );
            p.setOpacity( m_highlightedness );
            p.drawPixmap( 0, 0, Render::renderElement( "pile_selected", size ) );
        }
        else
        {
            QString id = isHighlighted() ? "pile_selected" : "pile";
            pix = Render::renderElement( id, cardScene()->deck()->cardSize() );
        }
    }

    setPixmap( pix );
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
            return cardScene()->checkAdd( checkIndex(), this, _cards );
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
            return cardScene()->checkRemove( checkIndex(), this, c);
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
        if ( card->animated() || cardScene()->cardsBeingDragged().contains( card ) ) {
            m_relayoutTimer->start( 50 );
            return;
        }
    }

    layoutCards( DURATION_RELAYOUT );
}

void Pile::add( Card *_card, int index)
{
    if (_card->source() == this)
        return;

    if (_card->scene() != scene())
        scene()->addItem(_card);

    Pile *oldSource = _card->source();
    if (oldSource)
    {
        _card->setTakenDown(oldSource->target() && !target());
        oldSource->remove(_card);
    }

    _card->setSource(this);
    _card->setVisible( isVisible() );

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
        QPointF offset( spread().width() * cardScene()->deck()->cardWidth(),
                        spread().height() * cardScene()->deck()->cardHeight() );
        if (!card->realFace())
            offset *= 0.6;
        return offset;
    }

    return QPointF(0, 0);
}

/* override cardtype (for initial deal ) */
void Pile::animatedAdd( Card* _card, bool faceUp )
{
    Q_ASSERT( _card );

    if ( _card->source() )
        _card->source()->relayoutCards();

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

void Pile::setHighlighted( bool flag )
{
    if ( flag != isHighlighted() )
    {
        HighlightableItem::setHighlighted( flag );

        m_fadeAnimation->setDirection( flag
                                       ? QAbstractAnimation::Forward
                                       : QAbstractAnimation::Backward );

        if ( m_fadeAnimation->state() != QAbstractAnimation::Running )
            m_fadeAnimation->start();
    }
}

void Pile::setHighlightedness( qreal highlightedness )
{
    m_highlightedness = highlightedness;
    updatePixmap();
    return;
}

qreal Pile::highlightedness() const
{
    return m_highlightedness;
}

void Pile::setGraphicVisible( bool flag ) {
    m_graphicVisible = flag;
    updatePixmap();
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

bool Pile::cardDoubleClicked(Card *c)
{
    emit doubleClicked(c);
    return false;
}

void Pile::layoutCards(int duration)
{
    if ( m_cards.isEmpty() )
        return;

    const QSize cardSize = cardScene()->deck()->cardSize();

    QPointF totalOffset( 0, 0 );
    for ( int i = 0; i < m_cards.size() - 1; ++i )
        totalOffset += cardOffset( m_cards[i] );

    qreal divx = 1;
    if ( totalOffset.x() )
        divx = qMin<qreal>( ( maximumSpace().width() - cardSize.width() ) / qAbs( totalOffset.x() ), 1.0 );

    qreal divy = 1;
    if ( totalOffset.y() )
        divy = qMin<qreal>( ( maximumSpace().height() - cardSize.height() ) / qAbs( totalOffset.y() ), 1.0 );

    QPointF cardPos = pos();
    qreal z = zValue() + 1;
    foreach ( Card * card, m_cards )
    {
        card->animate( cardPos, z, 1, 0, card->isFaceUp(), false, duration );

        QPointF offset = cardOffset( card );
        cardPos.rx() += divx * offset.x();
        cardPos.ry() += divy * offset.y();
        ++z;
    }
}

#include "pile.moc"
