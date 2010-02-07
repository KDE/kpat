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

#include "kcardpile.h"

#include "kcardscene.h"

#include <KDebug>

#include <QtCore/QTimer>
#include <QtCore/QPropertyAnimation>
#include <QtGui/QPainter>

#include <cmath>


KCardPile::KCardPile( const QString & objectName )
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

    m_fadeAnimation = new QPropertyAnimation( this, "highlightedness", this );
    m_fadeAnimation->setDuration( DURATION_CARDHIGHLIGHT );
    m_fadeAnimation->setKeyValueAt( 0, 0 );
    m_fadeAnimation->setKeyValueAt( 1, 1 );
}


KCardPile::~KCardPile()
{
//     dscene()->removePile(this);

    foreach ( KCard * c, m_cards )
        c->setSource( 0 );
}


int KCardPile::type() const
{
    return KCardPile::Type;
}


KCardScene * KCardPile::cardScene() const
{
    return dynamic_cast<KCardScene*>( scene() );
}


QList<KCard*> KCardPile::cards() const
{
    return m_cards;
}


int KCardPile::count() const
{
    return m_cards.count();
}


bool KCardPile::isEmpty() const
{
    return m_cards.isEmpty();
}


int KCardPile::indexOf( const KCard * card ) const
{
    return m_cards.indexOf( const_cast<KCard*>( card ) );
}


KCard * KCardPile::at( int index ) const
{
    if ( index < 0 || index >= m_cards.size() )
        return 0;
    return m_cards.at( index );
}


KCard *KCardPile::top() const
{
    if ( m_cards.isEmpty() )
        return 0;

    return m_cards.last();
}


QList<KCard*> KCardPile::topCardsDownTo( const KCard * card ) const
{
    int index = m_cards.indexOf( const_cast<KCard*>( card ) );
    if ( index == -1 )
        return QList<KCard*>();
    return m_cards.mid( index );
}


void KCardPile::setPilePos( QPointF pos )
{
    m_pilePos = pos;
}


void KCardPile::setPilePos( qreal x,  qreal y )
{
    setPilePos( QPointF( x, y ) );
}


QPointF KCardPile::pilePos() const
{
    return m_pilePos;
}


void KCardPile::setReservedSpace( QSizeF space )
{
    m_reserved = space;
}


void KCardPile::setReservedSpace( qreal width, qreal height )
{
    setReservedSpace( QSizeF( width, height ) );
}


QSizeF KCardPile::reservedSpace() const
{
    return m_reserved;
}


void KCardPile::setMaximumSpace( QSizeF size )
{
    m_maximumSpace = size;
}


QSizeF KCardPile::maximumSpace() const
{
    return m_maximumSpace;
}


void KCardPile::setSpread( QSizeF spread )
{
    m_spread = spread;
}


void KCardPile::setSpread( qreal width, qreal height )
{
    setSpread( QSizeF( width, height ) );
}


QSizeF KCardPile::spread() const
{
    return m_spread;
}


void KCardPile::setAutoTurnTop( bool autoTurnTop )
{
    m_autoTurnTop = autoTurnTop;
}


bool KCardPile::autoTurnTop() const
{
    return m_autoTurnTop;
}


void KCardPile::setVisible( bool visible )
{
    if ( visible != isVisible() )
    {
        QGraphicsItem::setVisible( visible );
        foreach ( KCard * c, m_cards )
            c->setVisible( visible );
    }
}


void KCardPile::setHighlighted( bool highlighted )
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


bool KCardPile::isHighlighted() const
{
    return m_highlighted;
}


void KCardPile::setGraphicVisible( bool visible )
{
    if ( m_graphicVisible != visible )
    {
        m_graphicVisible = visible;
        updatePixmap( pixmap().size() );
    }
}


bool KCardPile::isGraphicVisible()
{
    return m_graphicVisible;
}


void KCardPile::setGraphicSize( QSize size )
{
    if ( size != pixmap().size() )
        updatePixmap( size );
}


void KCardPile::animatedAdd( KCard * card, bool faceUp )
{
    Q_ASSERT( card );

    QPointF origPos = card->pos();
    KCardPile * origPile = card->source();

    card->turn( faceUp );
    add( card );

    if ( origPile )
        origPile->layoutCards();
    layoutCards();

    card->completeAnimation();
    QPointF destPos = card->pos();
    card->setPos( origPos );

    QPointF delta = destPos - card->pos();
    qreal dist = sqrt( delta.x() * delta.x() + delta.y() * delta.y() );
    qreal whole = sqrt( scene()->width() * scene()->width() + scene()->height() * scene()->height() );
    card->moveTo(destPos, card->zValue(), qRound( dist * DURATION_DEAL / whole ) );
}


void KCardPile::add( KCard * card, int index )
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


void KCardPile::remove( KCard * card )
{
    Q_ASSERT( m_cards.contains( card ) );
    m_cards.removeAll( card );
    card->setSource( 0 );
}


void KCardPile::clear()
{
    foreach ( KCard *card, m_cards )
        remove( card );
}


void KCardPile::layoutCards( int duration )
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

    for ( int i = 0; i < m_cards.size() - 1; ++i )
    {
        KCard * card = m_cards[i];
        card->animate( cardPos, z, 1, 0, card->isFaceUp(), false, duration );

        QPointF offset = cardOffset( card );
        cardPos.rx() += divx * offset.x();
        cardPos.ry() += divy * offset.y();
        ++z;
    }

    if ( m_autoTurnTop && !top()->isFaceUp() )
        top()->animate( cardPos, z, 1, 0, true, false, DURATION_FLIP );
    else
        top()->animate( cardPos, z, 1, 0, top()->isFaceUp(), false, duration );
}


void KCardPile::moveCardsBack( QList<KCard*> & cards, int duration )
{
    if ( cards.isEmpty() )
        return;

    foreach ( KCard * c, cards )
        c->raise();

    layoutCards( duration );
}


void KCardPile::moveCards( QList<KCard*> & cards, KCardPile * pile )
{
    if ( cards.isEmpty() )
        return;

    foreach ( KCard * c, cards )
    {
        Q_ASSERT( c->source() == this );
        pile->add( c );
    }

    layoutCards();

    pile->moveCardsBack( cards );
}


void KCardPile::cardPressed( KCard * card )
{
    emit pressed( card );
}


bool KCardPile::cardClicked( KCard * card )
{
    emit clicked( card );
    return false;
}


bool KCardPile::cardDoubleClicked( KCard * card )
{
    emit doubleClicked( card );
    return false;
}


// Return the number of pixels in x and y that the card should be
// offset from the start position of the pile.
QPointF KCardPile::cardOffset( const KCard * card ) const
{
    QPointF offset( spread().width() * card->pixmap().width(),
                    spread().height() * card->pixmap().height() );
    if (!card->realFace())
        offset *= 0.6;
    return offset;
}


QPixmap KCardPile::normalPixmap( QSize size )
{
    QPixmap pix( size );
    pix.fill( Qt::transparent );
    return pix;
}


QPixmap KCardPile::highlightedPixmap( QSize size )
{
    QPixmap pix( size );
    pix.fill( QColor( 0, 0, 0, 128 ) );
    return pix;
}


void KCardPile::updatePixmap( QSize size )
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

void KCardPile::setHighlightedness( qreal highlightedness )
{
    if ( m_highlightedness != highlightedness )
    {
        m_highlightedness = highlightedness;
        updatePixmap( pixmap().size() );
    }
}

qreal KCardPile::highlightedness() const
{
    return m_highlightedness;
}


#include "kcardpile.moc"
