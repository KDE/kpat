/*
 * Copyright (C) 2008-2009 Stephan Kulow <coolo@kde.org>
 * Copyright (C) 2008-2009 Parker Coates <parker.coates@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of 
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gameselectionscene.h"

#include "dealerinfo.h"
#include "renderer.h"

#include <KColorUtils>
#include <KDebug>
#include <KLocale>

#include <QtGui/QGraphicsObject>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtGui/QStyleOptionGraphicsItem>
#include <QtCore/QPropertyAnimation>

#include <cmath>


const qreal boxPaddingRatio = 0.037;
const qreal spacingRatio = 0.10;
const qreal textToTotalHeightRatio = 1 / 6.0;
const int hoverTransitionDuration = 300;


class GameSelectionScene::GameSelectionBox : public QGraphicsObject
{
    Q_OBJECT
    Q_PROPERTY( qreal fade READ hoverFadeAmount WRITE setHoverFadeAmount )

public:
    GameSelectionBox( const QString & name, int id )
      : m_label( name ),
        m_gameId( id ),
        m_anim( new QPropertyAnimation( this, "fade", this ) ),
        m_highlightFadeAmount( 0 )
    {
        setAcceptHoverEvents( true );
        m_anim->setDuration( hoverTransitionDuration );
        m_anim->setStartValue( qreal( 0.0 ) );
        m_anim->setEndValue( qreal( 1.0 ) );
        m_anim->setEasingCurve( QEasingCurve::InOutSine );
    }

    void setSize( const QSize & size )
    {
        m_size = size;
    }

    virtual QRectF boundingRect() const
    {
        return QRectF( QPointF(), m_size );
    }

    QString label() const
    {
        return m_label;
    }

    int id() const
    {
        return m_gameId;
    }

    void setHighlighted( bool highlighted )
    {
        if ( highlighted )
        {
            m_anim->setDirection( QAbstractAnimation::Forward );
            if ( m_anim->state() != QAbstractAnimation::Running )
                m_anim->start();
        }
        else
        {
            m_anim->setDirection( QAbstractAnimation::Backward );
            if ( m_anim->state() != QAbstractAnimation::Running )
                m_anim->start();
        }
    }

    static bool lessThan( const GameSelectionBox * a, const GameSelectionBox * b )
    {
        return a->m_label < b->m_label;
    }

signals:
    void selected( int gameId );
    void hoverChanged( GameSelectionBox * box, bool hovered );

protected:
    virtual void mousePressEvent( QGraphicsSceneMouseEvent * event )
    {
        Q_UNUSED( event )
        emit selected( m_gameId );
    }

    virtual void hoverEnterEvent( QGraphicsSceneHoverEvent * event )
    {
        Q_UNUSED( event )
        emit hoverChanged( this, true );
    }

    virtual void hoverLeaveEvent( QGraphicsSceneHoverEvent * event )
    {
        Q_UNUSED( event )
        emit hoverChanged( this, false );
    }

    qreal hoverFadeAmount() const
    {
        return m_highlightFadeAmount;
    }

    void setHoverFadeAmount( qreal amount )
    {
        m_highlightFadeAmount = amount;
        update();
    }

    virtual void paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 )
    {
        Q_UNUSED( option )
        Q_UNUSED( widget )
        int textAreaHeight = m_size.height() * textToTotalHeightRatio;
        int padding = boxPaddingRatio * m_size.width();
        QSize previewSize( m_size.height() - padding * 2, m_size.height() - padding * 2 - textAreaHeight );
        QRect textRect( 0, 0, m_size.width(), textAreaHeight );

        Renderer * r = Renderer::self();

        // Draw background/frame
        if ( m_anim->state() == QAbstractAnimation::Running )
        {
            qreal opacity = painter->opacity();
            painter->setOpacity( opacity * ( 1.0 - m_highlightFadeAmount ) );
            painter->drawPixmap( QPointF( 0, 0 ),
                                 r->renderElement( "bubble", m_size ) );
            painter->setOpacity( opacity * m_highlightFadeAmount );
            painter->drawPixmap( QPointF( 0, 0 ),
                                 r->renderElement( "bubble_hover", m_size ) );
            painter->setOpacity( opacity );
        }
        else if ( m_highlightFadeAmount == 1 )
        {
            painter->drawPixmap( 0, 0, r->renderElement( "bubble_hover", m_size ) );
        }
        else
        {
            painter->drawPixmap( 0, 0, r->renderElement( "bubble", m_size ) );
        }

        // Draw game preview
        painter->drawPixmap( ( m_size.width() - previewSize.width() ) / 2,
                             padding + textAreaHeight,
                             r->renderGamePreview( m_gameId, previewSize ) );

        // Draw label
        painter->setFont( scene()->font() );
        painter->setPen( KColorUtils::mix( r->colorOfElement( "bubble_text_color" ),
                                           r->colorOfElement( "bubble_hover_text_color" ), 
                                           m_highlightFadeAmount ) );
        painter->drawText( textRect, Qt::AlignCenter, m_label );
    }

private:
    QString m_label;
    int m_gameId;
    QSize m_size;
    QPropertyAnimation *m_anim;
    qreal m_highlightFadeAmount;
};


GameSelectionScene::GameSelectionScene( QObject * parent )
  : QGraphicsScene( parent ),
    m_selectionIndex( -1 )
{
    foreach (const DealerInfo * i, DealerInfoList::self()->games())
    {
        GameSelectionBox * box = new GameSelectionBox( i18n( i->name() ), i->ids().first() );
        m_boxes.append( box );
        addItem( box );

        connect( box, SIGNAL(selected(int)), this, SIGNAL(gameSelected(int)) );
        connect( box, SIGNAL(hoverChanged(GameSelectionBox*,bool)), this, SLOT(boxHoverChanged(GameSelectionBox*,bool)) );
    }

    qSort( m_boxes.begin(), m_boxes.end(), GameSelectionBox::lessThan );
}


GameSelectionScene::~GameSelectionScene()
{
}


void GameSelectionScene::resizeScene( const QSize & size )
{
    int numBoxes = m_boxes.size();
    QSizeF boxSize( Renderer::self()->sizeOfElement( "bubble" ) );
    qreal boxAspect = boxSize.width() / boxSize.height();
    qreal sceneAspect = qreal( size.width() ) / size.height();

    // Determine the optimal number of rows/columns for the grid
    m_columns = 1;
    int numRows = 1;
    int bestNumRows = 1;
    qreal lowestError = 10e10;
    for ( numRows = 1; numRows <= numBoxes; ++numRows )
    {
        m_columns = ceil( qreal( numBoxes ) / numRows );
        int numNonEmptyRows = ceil( qreal( numBoxes ) / m_columns );
        qreal gridAspect = boxAspect * m_columns / numNonEmptyRows;
        qreal error = gridAspect > sceneAspect ? gridAspect / sceneAspect
                                               : sceneAspect / gridAspect;
        if ( error < lowestError )
        {
            lowestError = error;
            bestNumRows = numRows;
        }
    }
    numRows = bestNumRows;
    m_columns = ceil( qreal( numBoxes ) / bestNumRows );

    // Calculate the box and grid dimensions
    qreal gridAspect = boxAspect * ( m_columns + spacingRatio * ( m_columns + 1 ) )
                                  / ( numRows + spacingRatio * ( numRows + 1 ) );
    int boxWidth, boxHeight;
    if ( gridAspect > sceneAspect )
    {
        boxWidth = size.width() / ( m_columns + spacingRatio * ( m_columns + 1 ) );
        boxHeight = boxWidth / boxAspect;
    }
    else
    {
        boxHeight = size.height() / ( numRows + spacingRatio * ( numRows + 1 ) );
        boxWidth = boxHeight * boxAspect;
    }
    int gridWidth = boxWidth * ( m_columns + spacingRatio * ( m_columns + 1 ) );
    int gridHeight = boxHeight * ( numRows + spacingRatio * ( numRows + 1 ) );

    // Set up the sceneRect so that the grid is centered
    setSceneRect( ( gridWidth - size.width() ) / 2 - boxWidth * spacingRatio,
                  ( gridHeight - size.height() ) / 2 - boxHeight * spacingRatio,
                  size.width(),
                  size.height() );

    // Initial font size estimate
    QPainter p;
    int maxLabelWidth = boxWidth * ( 1 - 2 * boxPaddingRatio );
    int pixelFontSize = boxHeight * (textToTotalHeightRatio - 1.5 * boxPaddingRatio);
    QFont f;
    f.setPixelSize( pixelFontSize );
    p.setFont( f );

    int row = 0;
    int col = 0;
    foreach ( GameSelectionBox * box, m_boxes )
    {
        // Reduce font size until the label fits
        while ( pixelFontSize > 0 &&
                p.boundingRect( QRectF(), box->label() ).width() > maxLabelWidth )
        {
            f.setPixelSize( --pixelFontSize );
            p.setFont( f );
        }

        // Position and size the boxes
        box->setPos( col * ( boxWidth * ( 1 + spacingRatio ) ),
                      row * ( boxHeight * ( 1 + spacingRatio ) ) );
        box->setSize( QSize( boxWidth, boxHeight ) );

        // Increment column and row
        ++col;
        if ( col == m_columns )
        {
            col = 0;
            ++row;
        }
    }

    setFont( f );
}


void GameSelectionScene::keyReleaseEvent( QKeyEvent * event )
{
    if ( m_selectionIndex == -1 )
    {
        m_selectionIndex = 0;
        m_boxes.at( m_selectionIndex )->setHighlighted( true );
    }
    else if ( event->key() == Qt::Key_Up
         && m_selectionIndex / m_columns > 0 )
    {
        m_boxes.at( m_selectionIndex )->setHighlighted( false );
        m_selectionIndex -= m_columns;
        m_boxes.at( m_selectionIndex )->setHighlighted( true );
    }
    else if ( event->key() == Qt::Key_Down
              && m_selectionIndex + m_columns < m_boxes.size() )
    {
        m_boxes.at( m_selectionIndex )->setHighlighted( false );
        m_selectionIndex += m_columns;
        m_boxes.at( m_selectionIndex )->setHighlighted( true );
    }
    else if ( event->key() == Qt::Key_Left
              && m_selectionIndex % m_columns > 0 )
    {
        m_boxes.at( m_selectionIndex )->setHighlighted( false );
        --m_selectionIndex;
        m_boxes.at( m_selectionIndex )->setHighlighted( true );
    }
    else if ( event->key() == Qt::Key_Right
              && m_selectionIndex % m_columns < m_columns - 1
              && m_selectionIndex < m_boxes.size() - 1 )
    {
        m_boxes.at( m_selectionIndex )->setHighlighted( false );
        ++m_selectionIndex;
        m_boxes.at( m_selectionIndex )->setHighlighted( true );
    }
    else if ( ( event->key() == Qt::Key_Return
                || event->key() ==Qt::Key_Enter
                || event->key() ==Qt::Key_Space )
              && m_selectionIndex != -1 )
    {
        emit gameSelected( m_boxes.at( m_selectionIndex )->id() );
    }
}


void GameSelectionScene::boxHoverChanged( GameSelectionScene::GameSelectionBox * box, bool hovered )
{
    if ( hovered )
    {
        if ( m_selectionIndex != -1 )
            m_boxes.at( m_selectionIndex )->setHighlighted( false );

        m_selectionIndex = m_boxes.indexOf( box );
        box->setHighlighted( true );
    }
    else
    {
        if ( m_boxes.indexOf( box ) == m_selectionIndex )
        {
            m_selectionIndex = -1;
            box->setHighlighted( false );
        }
    }
}


#include "gameselectionscene.moc"
#include "moc_gameselectionscene.cpp"
