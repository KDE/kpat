/*
 * Copyright (C) 1995 Paul Olav Tvete <paul@troll.no>
 * Copyright (C) 2000-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2010 Parker Coates <parker.coates@kdemail.net>
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

#include <KDebug>

#include <QtCore/QTimer>
#include <QtCore/QPropertyAnimation>
#include <QtGui/QPainter>

#include <cmath>


Pile::Pile( const QString & objectName )
  : QGraphicsPixmapItem(),
    m_autoTurnTop(false),
    m_highlighted( false ),
    m_graphicVisible( true ),
    m_reserved( 1, 1 ),
    m_spread( 0, 0.33 )
{
    setObjectName( objectName );

    setZValue( 0 );
    setShapeMode( QGraphicsPixmapItem::BoundingRectShape );
    QGraphicsItem::setVisible( true );

    setMaximumSpace( QSizeF( 1, 1 ) ); // just to make it valid

    m_relayoutTimer = new QTimer( this );
    m_relayoutTimer->setSingleShot( true );
    connect( m_relayoutTimer, SIGNAL(timeout()), SLOT(relayoutCards()) );

    m_fadeAnimation = new QPropertyAnimation( this, "highlightedness", this );
    m_fadeAnimation->setDuration( DURATION_CARDHIGHLIGHT );
    m_fadeAnimation->setKeyValueAt( 0, 0 );
    m_fadeAnimation->setKeyValueAt( 1, 1 );
}


Pile::~Pile()
{
//     dscene()->removePile(this);

    foreach ( Card * c, m_cards )
        c->setSource( 0 );

    delete m_relayoutTimer;
}


int Pile::type() const
{
    return Type;
}


CardScene * Pile::cardScene() const
{
    return dynamic_cast<CardScene*>( scene() );
}


QList<Card*> Pile::cards() const
{
    return m_cards;
}


int Pile::count() const
{
    return m_cards.count();
}


bool Pile::isEmpty() const
{
    return m_cards.isEmpty();
}


int Pile::indexOf( const Card * card ) const
{
    return m_cards.indexOf( const_cast<Card*>( card ) );
}


Card * Pile::at( int index ) const
{
    if ( index < 0 || index >= m_cards.size() )
        return 0;
    return m_cards.at( index );
}


Card *Pile::top() const
{
    if ( m_cards.isEmpty() )
        return 0;

    return m_cards.last();
}


QList<Card*> Pile::topCardsDownTo( const Card * card ) const
{
    int index = m_cards.indexOf( const_cast<Card*>( card ) );
    if ( index == -1 )
        return QList<Card*>();
    return m_cards.mid( index );
}


void Pile::setPilePos( QPointF pos )
{
    m_pilePos = pos;
}


void Pile::setPilePos( qreal x,  qreal y )
{
    setPilePos( QPointF( x, y ) );
}


QPointF Pile::pilePos() const
{
    return m_pilePos;
}


void Pile::setReservedSpace( QSizeF space )
{
    m_reserved = space;
}


void Pile::setReservedSpace( qreal width, qreal height )
{
    setReservedSpace( QSizeF( width, height ) );
}


QSizeF Pile::reservedSpace() const
{
    return m_reserved;
}


void Pile::setMaximumSpace( QSizeF size )
{
    m_maximumSpace = size;
}


QSizeF Pile::maximumSpace() const
{
    return m_maximumSpace;
}


void Pile::setSpread( QSizeF spread )
{
    m_spread = spread;
}


void Pile::setSpread( qreal width, qreal height )
{
    setSpread( QSizeF( width, height ) );
}


QSizeF Pile::spread() const
{
    return m_spread;
}


void Pile::setAutoTurnTop( bool autoTurnTop )
{
    m_autoTurnTop = autoTurnTop;
}


bool Pile::autoTurnTop() const
{
    return m_autoTurnTop;
}


void Pile::setVisible( bool visible )
{
    if ( visible != isVisible() )
    {
        QGraphicsItem::setVisible( visible );
        foreach ( Card * c, m_cards )
            c->setVisible( visible );
    }
}


void Pile::setHighlighted( bool highlighted )
{
    if ( highlighted != m_highlighted )
    {
        m_highlighted = highlighted;
        m_fadeAnimation->setDirection( highlighted
                                       ? QAbstractAnimation::Forward
                                       : QAbstractAnimation::Backward );
        if ( m_fadeAnimation->state() != QAbstractAnimation::Running )
            m_fadeAnimation->start();
    }
}


bool Pile::isHighlighted() const
{
    return m_highlighted;
}


void Pile::setGraphicVisible( bool visible )
{
    if ( m_graphicVisible != visible )
    {
        m_graphicVisible = visible;
        updatePixmap( pixmap().size() );
    }
}


bool Pile::isGraphicVisible()
{
    return m_graphicVisible;
}


void Pile::setGraphicSize( QSize size )
{
    if ( size != pixmap().size() )
        updatePixmap( size );
}


void Pile::animatedAdd( Card * card, bool faceUp )
{
    Q_ASSERT( card );

    if ( card->source() )
        card->source()->relayoutCards();

    QPointF origPos = card->pos();
    card->turn( faceUp );
    add( card );
    layoutCards( DURATION_RELAYOUT );

    card->completeAnimation();
    QPointF destPos = card->pos();
    card->setPos( origPos );

    QPointF delta = destPos - card->pos();
    qreal dist = sqrt( delta.x() * delta.x() + delta.y() * delta.y() );
    qreal whole = sqrt( scene()->width() * scene()->width() + scene()->height() * scene()->height() );
    card->moveTo(destPos, card->zValue(), qRound( dist * DURATION_DEAL / whole ) );
}


void Pile::add( Card * card, int index )
{
    if ( card->source() == this )
        return;

    if ( card->scene() != scene() )
        scene()->addItem(card);

    if ( card->source() )
        card->source()->remove( card );

    card->setSource( this );
    card->setVisible( isVisible() );

    if ( index == -1 )
    {
        m_cards.append( card );
    }
    else
    {
        while ( m_cards.count() <= index )
            m_cards.append( 0 );

        Q_ASSERT( m_cards[index] == 0 );
        m_cards[index] = card;
    }
}


void Pile::remove( Card * card )
{
    Q_ASSERT( m_cards.contains( card ) );
    m_cards.removeAll( card );
    card->setSource( 0 );
}


void Pile::clear()
{
    foreach ( Card *card, m_cards )
        remove( card );
}


void Pile::layoutCards( int duration )
{
    if ( m_cards.isEmpty() )
        return;

    const QSize cardSize = m_cards.first()->pixmap().size();

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


void Pile::moveCardsBack( QList<Card*> & cards, int duration )
{
    if ( cards.isEmpty() )
        return;

    foreach ( Card * c, cards )
        c->raise();

    layoutCards( duration );
}


void Pile::moveCards( QList<Card*> & cards, Pile * pile )
{
    if ( cards.isEmpty() )
        return;

    foreach ( Card * c, cards )
    {
        Q_ASSERT( c->source() == this );
        pile->add( c );
    }

    Card * t = top();
    if ( t && !t->isFaceUp() && m_autoTurnTop )
    {
        t->animate( t->pos(), t->zValue(), 1, 0, true, false, DURATION_FLIP );
    }

    relayoutCards();

    pile->moveCardsBack( cards );
}


void Pile::cardPressed( Card * card )
{
    emit pressed( card );
}


bool Pile::cardClicked( Card * card )
{
    emit clicked( card );
    return false;
}


bool Pile::cardDoubleClicked( Card * card )
{
    emit doubleClicked( card );
    return false;
}


void Pile::relayoutCards()
{
    m_relayoutTimer->stop();

    foreach ( Card * card, m_cards )
    {
        if ( card->animated() || cardScene()->cardsBeingDragged().contains( card ) )
        {
            m_relayoutTimer->start( 50 );
            return;
        }
    }

    layoutCards( DURATION_RELAYOUT );
}


// Return the number of pixels in x and y that the card should be
// offset from the start position of the pile.
QPointF Pile::cardOffset( const Card * card ) const
{
    QPointF offset( spread().width() * card->pixmap().width(),
                    spread().height() * card->pixmap().height() );
    if (!card->realFace())
        offset *= 0.6;
    return offset;
}


QPixmap Pile::normalPixmap( QSize size )
{
    QPixmap pix( size );
    pix.fill( Qt::transparent );
    return pix;
}


QPixmap Pile::highlightedPixmap( QSize size )
{
    QPixmap pix( size );
    pix.fill( QColor( 0, 0, 0, 128 ) );
    return pix;
}


void Pile::updatePixmap( QSize size )
{
    if ( !scene() )
        return;

    QPixmap pix;
    if ( m_graphicVisible )
    {
        if ( m_fadeAnimation->state() == QAbstractAnimation::Running )
        {
            pix = QPixmap( size );
            pix.fill( Qt::transparent );
            QPainter p( &pix );
            p.setOpacity( 1 - m_highlightedness );
            p.drawPixmap( 0, 0, normalPixmap( size ) );
            p.setOpacity( m_highlightedness );
            p.drawPixmap( 0, 0, highlightedPixmap( size ) );
        }
        else if ( m_highlighted )
        {
            pix = highlightedPixmap( size );
        }
        else
        {
            pix = normalPixmap( size );
        }
    }
    else
    {
        pix = QPixmap( size );
        pix.fill( Qt::transparent );
    }

    setPixmap( pix );
}

void Pile::setHighlightedness( qreal highlightedness )
{
    if ( m_highlightedness != highlightedness )
    {
        m_highlightedness = highlightedness;
        updatePixmap( pixmap().size() );
    }
}

qreal Pile::highlightedness() const
{
    return m_highlightedness;
}


#include "pile.moc"
